// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-14 | Exercise deferred evidence through the dispatcher pipe. | Preserve both native conflict rules at the actual privileged open boundary. | #731 | gpt-6.1-terra |
// | 2026-11-14 | Check both native deferred-conflict evidence forms. | Distinguish legacy identity from the existing full revision comparison. | #731 | gpt-6.1-terra |
// | 2026-11-14 | Reject a write-only reopen flag on other operations. | Keep the handle-access contract bound to a verified reopen. | #731 | gpt-6.1-terra |
// | 2026-11-14 | Compare stream queries on native and transferred handles. | Verify the real worker can execute its existing ADS cleanup step. | #731 | gpt-6.1-terra |
// | 2026-11-14 | Compare scoped overwrite with the native file open. | Exercise actual NTFS and dispatcher behavior before worker integration. | #731 | GPT-6 |

// Real pipe and dispatcher, same-token fixture. This does not run DoCopyFile
// or Windows UAC and must not be reported as full F5 acceptance.
struct ThinOverwriteSnapshot
{
    FILE_ID_INFO id{};
    FILE_BASIC_INFO basic{};
    LONGLONG size = -1;
    std::wstring security;

    bool Read(const std::wstring& path)
    {
        auto file = Adopt(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES | READ_CONTROL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        FILE_STANDARD_INFO standard{};
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        constexpr DWORD fields = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
        if (!file || !GetFileInformationByHandleEx(file.get(), FileIdInfo, &id, sizeof(id)) ||
            !GetFileInformationByHandleEx(file.get(), FileBasicInfo, &basic, sizeof(basic)) ||
            !GetFileInformationByHandleEx(file.get(), FileStandardInfo, &standard, sizeof(standard)) ||
            GetSecurityInfo(file.get(), SE_FILE_OBJECT, fields, nullptr, nullptr, nullptr, nullptr, &descriptor))
            return false;
        const std::unique_ptr<void, decltype(&LocalFree)> owned(descriptor, LocalFree);
        LPWSTR sddl = nullptr;
        if (!ConvertSecurityDescriptorToStringSecurityDescriptorW(descriptor, SDDL_REVISION_1, fields, &sddl, nullptr))
            return false;
        const std::unique_ptr<void, decltype(&LocalFree)> text(sddl, LocalFree);
        security = sddl;
        size = standard.EndOfFile.QuadPart;
        return true;
    }
};

void CheckThinOverwrite(const Directory& source, const Directory& target, const Directory& outside)
{
    const wchar_t* sourceLeaf = L"overwrite-source-東.txt";
    Check(WriteByte(source.File(sourceLeaf), 0x42), "prepare overwrite source");
    auto input = Adopt(CreateFileW(source.File(sourceLeaf).c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
    FILE_ID_INFO sourceId{};
    if (!input || !GetFileInformationByHandleEx(input.get(), FileIdInfo, &sourceId, sizeof(sourceId)))
    { Check(false, "identify overwrite source"); return; }
    Directory nested, deep;
    if (!nested.Open(target.File(std::wstring(120, L'x') + L"-東").c_str()) ||
        !deep.Open(nested.File(std::wstring(120, L'y') + L"-日").c_str()))
    { Check(false, "prepare long Unicode overwrite parent"); return; }
    Check(deep.path.size() > 260, "overwrite uses actual long Unicode parent");

    for (bool asynchronous : {false, true})
    {
        for (DWORD bits = 1; bits < 8; ++bits)
        {
            const DWORD attributes = FILE_ATTRIBUTE_ARCHIVE |
                ((bits & 1) ? FILE_ATTRIBUTE_READONLY : 0) |
                ((bits & 2) ? FILE_ATTRIBUTE_HIDDEN : 0) |
                ((bits & 4) ? FILE_ATTRIBUTE_SYSTEM : 0);
            const std::wstring leaf = std::wstring(asynchronous ? L"async-native-source-" : L"sync-native-source-") +
                                      std::to_wstring(bits) + L"-東";
            const auto path = source.File(leaf);
            Check(WriteByte(path, 0x47) && SetFileAttributesW(path.c_str(), attributes),
                  "prepare source with native read-only hidden or system attributes");
            auto identityHandle = Adopt(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
            FILE_ID_INFO identity{};
            if (!identityHandle || !GetFileInformationByHandleEx(identityHandle.get(), FileIdInfo, &identity, sizeof(identity)))
            { Check(false, "capture native-attribute source identity"); return; }
            identityHandle.reset();
            JobPipe job;
            DWORD status = ERROR_GEN_FAILURE;
            ThinWireReply reply{};
            const bool opened = job.Open() && job.Thin(source, leaf, identity, deep, leaf,
                Operation::ThinOpenSource, nullptr, asynchronous, status, reply, INVALID_HANDLE_VALUE,
                INVALID_HANDLE_VALUE, false, 28u);
            auto reader = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
            Check(opened && !status && reader, "native-attribute source is opened within approved scope");
            BYTE value = 0; DWORD bytes = 0;
            OVERLAPPED io{};
            auto event = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            io.hEvent = event.get();
            BOOL read = reader && ReadFile(reader.get(), &value, 1, &bytes, asynchronous ? &io : nullptr);
            if (!read && asynchronous && GetLastError() == ERROR_IO_PENDING)
                read = GetOverlappedResult(reader.get(), &io, &bytes, TRUE);
            Check(read && bytes == 1 && value == 0x47, "delegated source retains native read semantics");
            Check(job.Thin(source, leaf, identity, deep, leaf, Operation::ThinTargetCreate,
                nullptr, asynchronous, status, reply, reader.get(), INVALID_HANDLE_VALUE, false, 28u) &&
                !status && reply.value, "source attributes do not block approved target creation");
            const FILE_ID_INFO targetId = reply.target;
            auto output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
            output.reset();
            Check(job.Thin(source, leaf, identity, deep, leaf, Operation::ThinDiscardTarget,
                &targetId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, 28u) &&
                !status && IsAbsent(deep.File(leaf)), "cleanup preserves source and removes only owned target");
            reader.reset(); job.Finish();
            Check(SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL) && DeleteFileW(path.c_str()),
                  "remove only native-attribute source fixture");
        }
    }

    {
        const auto path = deep.File(L"scan-evidence-東.txt");
        const auto parked = deep.File(L"scan-evidence-parked.txt");
        Check(WriteByte(path, 0x21), "prepare Scan Ahead target");
        auto anchor = Adopt(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
        BY_HANDLE_FILE_INFORMATION info{};
        ThinScanEvidence legacy{}, revision{};
        legacy.kind = ThinScanEvidence::Kind::LegacyIdentity;
        revision.kind = ThinScanEvidence::Kind::Revision;
        if (!anchor || !GetFileInformationByHandle(anchor.get(), &info) ||
            UacZipUpdate::ReadRevision(anchor.get(), revision.revision))
        { Check(false, "capture Scan Ahead evidence"); return; }
        legacy.legacyVolume = info.dwVolumeSerialNumber;
        legacy.legacyIndex = (static_cast<ULONGLONG>(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
        Check(!CheckThinScanEvidence(anchor.get(), legacy) && !CheckThinScanEvidence(anchor.get(), revision),
            "both native evidence forms accept their original target");
        auto writer = Adopt(CreateFileW(path.c_str(), GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
        LARGE_INTEGER size{}; size.QuadPart = 3;
        Check(writer && SetFilePointerEx(writer.get(), size, nullptr, FILE_BEGIN) && SetEndOfFile(writer.get()),
            "change original target size without changing its identity");
        writer.reset();
        Check(!CheckThinScanEvidence(anchor.get(), legacy) &&
              CheckThinScanEvidence(anchor.get(), revision) == ERROR_FILE_INVALID,
            "legacy identity tolerates content change while revision rejects it");
        Check(MoveFileW(path.c_str(), parked.c_str()) && WriteByte(path, 0x22),
            "replace Scan Ahead pathname");
        auto replacement = Adopt(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
        Check(replacement && CheckThinScanEvidence(replacement.get(), legacy) == ERROR_FILE_INVALID &&
              CheckThinScanEvidence(replacement.get(), revision) == ERROR_FILE_INVALID,
            "both evidence forms reject a replacement object");
        Handle deniedFile, deniedAnchor;
        bool overwritten = false;
        Check(OpenThinOverwriteTarget(deep.handle.get(), L"scan-evidence-東.txt", sourceId, false,
                deniedFile, deniedAnchor, overwritten, &legacy) == ERROR_REVISION_MISMATCH &&
              !overwritten && !deniedFile && HasByte(path, 0x22),
            "overwrite checks legacy identity before mutating replacement");
        deniedAnchor.reset();
        ThinOverwriteSnapshot preserved;
        Check(OpenThinOverwriteTarget(deep.handle.get(), L"scan-evidence-parked.txt", sourceId, false,
                deniedFile, deniedAnchor, overwritten, &revision) == ERROR_REVISION_MISMATCH &&
              !overwritten && !deniedFile && preserved.Read(parked) && preserved.size == 3,
            "overwrite checks full revision before mutating original");
        ThinScanEvidence invalid{};
        Check(CheckThinScanEvidence(anchor.get(), invalid) == ERROR_INVALID_PARAMETER,
            "unknown Scan Ahead evidence kind is refused");
        deniedAnchor.reset();
        Check(!OpenThinOverwriteTarget(deep.handle.get(), L"scan-evidence-parked.txt", sourceId, false,
                deniedFile, deniedAnchor, overwritten, &legacy) && overwritten && deniedFile && HasByte(path, 0x22),
            "matching legacy identity permits overwrite of the verified object");
    }

    for (bool fullRevision : {false, true})
    {
        for (int scenario = 0; scenario != 4; ++scenario)
        {
            const std::wstring leaf = std::wstring(fullRevision ? L"revision-" : L"legacy-") + std::to_wstring(scenario);
            const auto path = deep.File(leaf), parked = deep.File(leaf + L".parked");
            Check(WriteByte(path, 0x35), "prepare deferred conflict target");
            auto observed = Adopt(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
            BY_HANDLE_FILE_INFORMATION info{};
            ThinScanEvidence evidence{};
            evidence.kind = fullRevision ? ThinScanEvidence::Kind::Revision : ThinScanEvidence::Kind::LegacyIdentity;
            if (!observed || !GetFileInformationByHandle(observed.get(), &info) ||
                UacZipUpdate::ReadRevision(observed.get(), evidence.revision))
            { Check(false, "capture deferred conflict evidence"); return; }
            evidence.legacyVolume = info.dwVolumeSerialNumber;
            evidence.legacyIndex = (static_cast<ULONGLONG>(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
            if (scenario == 1)
                Check(MoveFileW(path.c_str(), parked.c_str()) && WriteByte(path, 0x36), "replace deferred target");
            if (scenario == 2)
            {
                auto writer = Adopt(CreateFileW(path.c_str(), GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
                LARGE_INTEGER size{}; size.QuadPart = 3;
                Check(writer && SetFilePointerEx(writer.get(), size, nullptr, FILE_BEGIN) && SetEndOfFile(writer.get()),
                    "change deferred target without replacing it");
            }
            if (scenario == 3)
                Check(MoveFileW(path.c_str(), parked.c_str()), "remove deferred target name");
            JobPipe job;
            DWORD status = ERROR_GEN_FAILURE;
            ThinWireReply reply{};
            const bool sent = job.Open() && job.Thin(source, sourceLeaf, sourceId, deep, leaf,
                Operation::ThinTargetOverwriteScan, nullptr, false, status, reply, input.get(),
                INVALID_HANDLE_VALUE, false, 28u, &evidence);
            const bool shouldMatch = scenario == 0 || (scenario == 2 && !fullRevision);
            auto output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
            Check(sent && (shouldMatch ? !status && output : status == ERROR_REVISION_MISMATCH && !output),
                "dispatcher validates native deferred evidence before overwrite");
            output.reset();
            if (scenario == 1)
                Check(HasByte(path, 0x36) && HasByte(parked, 0x35), "replacement and original stay intact");
            if (scenario == 3)
                Check(IsAbsent(path) && HasByte(parked, 0x35), "missing deferred name is not recreated");
            if (scenario == 2 && fullRevision)
            {
                ThinOverwriteSnapshot preserved;
                Check(preserved.Read(path) && preserved.size == 3, "changed revision is not truncated");
            }
            job.Finish();
        }
    }

    for (bool asynchronous : {false, true})
    {
        for (int scenario = 0; scenario < 4; ++scenario)
        {
            const std::wstring leaf = std::wstring(asynchronous ? L"async-" : L"sync-") +
                std::to_wstring(scenario) + L"-日.txt";
            const auto path = deep.File(leaf), baseline = deep.File(leaf + L".native");
            Check(WriteByte(path, 0x51) && WriteByte(path + L":data", 0x61) &&
                  WriteByte(baseline, 0x51) && WriteByte(baseline + L":data", 0x61),
                  "prepare native and broker target including ADS");
            const DWORD attrs = scenario == 1 ? FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM :
                FILE_ATTRIBUTE_ARCHIVE;
            Check(SetFileAttributesW(path.c_str(), attrs) && SetFileAttributesW(baseline.c_str(), attrs),
                "set identical input attributes");
            ThinOverwriteSnapshot before, nativeBefore, nativeAfter, after;
            if (!before.Read(path) || !nativeBefore.Read(baseline))
            { Check(false, "capture overwrite baseline"); return; }
            // The native worker clears these attributes after its native
            // confirmation, then calls CREATE_ALWAYS with exclusive sharing.
            if (scenario == 1) Check(SetFileAttributesW(baseline.c_str(), FILE_ATTRIBUTE_NORMAL) != FALSE,
                "native worker clears confirmed attributes");
            auto native = Adopt(CreateFileW(baseline.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                FILE_FLAG_SEQUENTIAL_SCAN | (asynchronous ? FILE_FLAG_OVERLAPPED : 0), nullptr));
            Check(native && nativeAfter.Read(baseline), "native CREATE_ALWAYS succeeds");
            BYTE nativeStreams[4096]{};
            IO_STATUS_BLOCK nativeIo{};
            const NTSTATUS nativeStreamStatus = NtQueryInformationFile(native.get(), &nativeIo,
                nativeStreams, sizeof(nativeStreams), static_cast<FILE_INFORMATION_CLASS>(22));
            native.reset();
            JobPipe job;
            DWORD status = ERROR_GEN_FAILURE;
            ThinWireReply reply{};
            Check(job.Open(), "open overwrite dispatcher session");
            if (!job.Thin(source, sourceLeaf, sourceId, deep, leaf, Operation::ThinTargetOverwrite,
                    nullptr, asynchronous, status, reply, input.get(), INVALID_HANDLE_VALUE, false, 28u) || status || !reply.value)
            { std::printf("overwrite status=%lu\n", status); Check(false, "overwrite returns real handle"); return; }
            auto output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
            BYTE brokerStreams[4096]{};
            IO_STATUS_BLOCK brokerIo{};
            const NTSTATUS brokerStreamStatus = NtQueryInformationFile(output.get(), &brokerIo,
                brokerStreams, sizeof(brokerStreams), static_cast<FILE_INFORMATION_CLASS>(22));
            Check(nativeStreamStatus == 0 && brokerStreamStatus == nativeStreamStatus,
                "transferred writer supports the same native stream query");
            const auto targetId = reply.target;
            Check(after.Read(path) && UacRenameLeaf::Detail::SameSource(before.id, after.id) &&
                UacRenameLeaf::Detail::SameSource(before.id, targetId) && after.size == 0 && nativeAfter.size == 0 &&
                before.basic.CreationTime.QuadPart == after.basic.CreationTime.QuadPart &&
                nativeBefore.basic.CreationTime.QuadPart == nativeAfter.basic.CreationTime.QuadPart &&
                before.security == after.security && nativeBefore.security == nativeAfter.security &&
                after.basic.FileAttributes == nativeAfter.basic.FileAttributes &&
                IsAbsent(path + L":data") && IsAbsent(baseline + L":data"),
                "overwrite matches native identity creation-time ACL attributes size and ADS semantics");
            auto competing = Adopt(CreateFileW(path.c_str(), GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
            Check(!competing && GetLastError() == ERROR_SHARING_VIOLATION, "transferred writer remains exclusive");
            BYTE data = 0x74;
            DWORD transferred = 0;
            OVERLAPPED io{};
            auto event = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            io.hEvent = event.get();
            Check(!ReadFile(output.get(), &data, 1, &transferred, asynchronous ? &io : nullptr) &&
                GetLastError() == ERROR_ACCESS_DENIED, "writer does not delegate read access");
            BOOL wrote = WriteFile(output.get(), &data, 1, &transferred, asynchronous ? &io : nullptr);
            if (!wrote && asynchronous && GetLastError() == ERROR_IO_PENDING)
                wrote = GetOverlappedResult(output.get(), &io, &transferred, TRUE);
            Check(wrote && transferred == 1, "host writes and drains actual sync or asynchronous I/O");
            output.reset();

            if (scenario == 2)
                Check(MoveFileW(path.c_str(), (path + L".parked").c_str()) && WriteByte(path, 0x7b),
                    "replace output after transfer while its identity remains anchored");
            else
            {
                Check(job.Thin(source, sourceLeaf, sourceId, deep, leaf, Operation::ThinTargetRetry,
                    &targetId, asynchronous, status, reply, input.get(), INVALID_HANDLE_VALUE, false, 28u) &&
                    !status && reply.value, "overwrite output uses real fresh Retry open");
                auto reopened = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
                data = 0;
                io = {}; io.hEvent = event.get(); ResetEvent(event.get());
                BOOL read = ReadFile(reopened.get(), &data, 1, &transferred, asynchronous ? &io : nullptr);
                if (!read && asynchronous && GetLastError() == ERROR_IO_PENDING)
                    read = GetOverlappedResult(reopened.get(), &io, &transferred, TRUE);
                Check(read && transferred == 1 && data == 0x74, "Retry starts at zero and can verify copied data");
            }
            if (scenario == 3)
            {
                Check(job.Thin(source, sourceLeaf, sourceId, deep, leaf, Operation::ThinTargetRelease,
                    &targetId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, 28u) && !status &&
                    HasByte(path, 0x74), "release preserves successful overwrite output");
            }
            else
            {
                Check(job.Thin(source, sourceLeaf, sourceId, deep, leaf, Operation::ThinDiscardTarget,
                    &targetId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, 28u) &&
                    !reply.value, "overwrite cleanup returns no handle");
                Check(scenario == 2 ? status == ERROR_FILE_INVALID && HasByte(path, 0x7b) && HasByte(path + L".parked", 0x74) :
                    !status && IsAbsent(path), "cleanup deletes owned output and preserves replacements");
            }
            job.Finish();
        }

        // Race the actual dispatcher after opening the metadata anchor. The
        // destructive open must address that object, never the new pathname.
        const std::wstring leaf = asynchronous ? L"race-async.txt" : L"race-sync.txt";
        const auto path = deep.File(leaf), parked = deep.File(leaf + L".parked");
        Check(WriteByte(path, 0x21) && WriteByte(path + L":data", 0x22), "prepare object-binding race");
        auto reached = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        auto resume = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        ThinIdentityScanPause pause{reached.get(), resume.get()};
        JobPipe job;
        Check(job.Open(nullptr, nullptr, Operation::FindContent, nullptr, false, nullptr, false, false, &pause),
            "open scheduled overwrite session");
        bool swapped = false;
        std::thread rename([&] {
            if (WaitForSingleObject(reached.get(), 5000) == WAIT_OBJECT_0)
                swapped = MoveFileW(path.c_str(), parked.c_str()) && WriteByte(path, 0x77);
            SetEvent(resume.get());
        });
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        const bool sent = job.Thin(source, sourceLeaf, sourceId, deep, leaf, Operation::ThinTargetOverwrite,
            nullptr, asynchronous, status, reply, input.get(), INVALID_HANDLE_VALUE, false, 28u);
        rename.join();
        auto output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
        ThinOverwriteSnapshot actual;
        Check(sent && !status && output && swapped && actual.Read(parked) && actual.size == 0 &&
            UacRenameLeaf::Detail::SameSource(actual.id, reply.target) && IsAbsent(parked + L":data") && HasByte(path, 0x77),
            "destructive reopen targets anchored object and leaves replacement intact");
        output.reset(); job.Finish();
    }

    // Compare the actual relative overwrite primitive with native CREATE_ALWAYS
    // before changing the production guard for special NTFS storage attributes.
    for (int variant = 0; variant != 4; ++variant)
    {
        const bool compressed = (variant & 1) != 0, asynchronous = (variant & 2) != 0;
        const std::wstring leaf = std::wstring(compressed ? L"special-compressed-" : L"special-sparse-") +
            (asynchronous ? L"async" : L"sync");
        const auto path = deep.File(leaf);
        const auto baseline = path + L".native";
        Check(WriteByte(path, 0x62) && WriteByte(baseline, 0x62) &&
              WriteByte(path + L":data", 0x64) && WriteByte(baseline + L":data", 0x64),
              "prepare special storage comparison fixtures");
        const auto setStorage = [compressed](const std::wstring& name) -> DWORD {
            auto file = Adopt(CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr));
            if (!file) return GetLastError();
            DWORD bytes = 0;
            USHORT format = COMPRESSION_FORMAT_DEFAULT;
            if (DeviceIoControl(file.get(), compressed ? FSCTL_SET_COMPRESSION : FSCTL_SET_SPARSE,
                    compressed ? &format : nullptr, compressed ? sizeof(format) : 0, nullptr, 0, &bytes, nullptr))
                return ERROR_SUCCESS;
            return GetLastError();
        };
        const DWORD actualSetup = setStorage(path), nativeSetup = setStorage(baseline);
        if (actualSetup || nativeSetup)
        {
            if (actualSetup == nativeSetup && (actualSetup == ERROR_NOT_SUPPORTED || actualSetup == ERROR_INVALID_FUNCTION))
                std::printf("SKIP: filesystem does not support this storage attribute (%lu)\n", actualSetup);
            else Check(false, "set equivalent special storage attributes");
            continue;
        }
        ThinOverwriteSnapshot before, after, nativeBefore, nativeAfter;
        Check(before.Read(path) && nativeBefore.Read(baseline), "capture special overwrite identity and security");
        auto native = Adopt(CreateFileW(baseline.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                        FILE_FLAG_SEQUENTIAL_SCAN | (asynchronous ? FILE_FLAG_OVERLAPPED : 0), nullptr));
        Check(native && nativeAfter.Read(baseline), "native special target overwrite succeeds");
        native.reset();
        JobPipe job;
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        Check(job.Open() && job.Thin(source, sourceLeaf, sourceId, deep, leaf,
            Operation::ThinTargetProbe, nullptr, false, status, reply, input.get(),
            INVALID_HANDLE_VALUE, false, 28u) && !status && reply.value,
            "special target supplies metadata for native conflict dialog");
        auto probe = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
        FILE_ATTRIBUTE_TAG_INFO probeAttributes{};
        Check(probe && GetFileInformationByHandleEx(probe.get(), FileAttributeTagInfo,
              &probeAttributes, sizeof(probeAttributes)) &&
              (probeAttributes.FileAttributes & (compressed ? FILE_ATTRIBUTE_COMPRESSED : FILE_ATTRIBUTE_SPARSE_FILE)),
              "metadata probe retains the original storage attribute");
        probe.reset();
        const bool sent = job.Thin(source, sourceLeaf, sourceId, deep, leaf,
            Operation::ThinTargetOverwrite, nullptr, asynchronous, status, reply, input.get(),
            INVALID_HANDLE_VALUE, false, 28u);
        const FILE_ID_INFO targetId = reply.target;
        auto output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
        Check(sent && !status && output && after.Read(path), "dispatcher overwrites special target");
        output.reset();
        Check(UacRenameLeaf::Detail::SameSource(before.id, after.id) &&
              UacRenameLeaf::Detail::SameSource(nativeBefore.id, nativeAfter.id) &&
              before.security == after.security && nativeBefore.security == nativeAfter.security &&
              after.size == nativeAfter.size && after.basic.FileAttributes == nativeAfter.basic.FileAttributes &&
              IsAbsent(path + L":data") && IsAbsent(baseline + L":data"),
              "special relative overwrite matches native identity security attributes size and ADS");
        Check(job.Thin(source, sourceLeaf, sourceId, deep, leaf, Operation::ThinTargetRetry,
            &targetId, asynchronous, status, reply, input.get(), INVALID_HANDLE_VALUE, false, 28u) &&
            !status && reply.value, "special target permits a fresh Retry open");
        auto retry = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
        retry.reset();
        Check(job.Thin(source, sourceLeaf, sourceId, deep, leaf, Operation::ThinTargetRelease,
            &targetId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, 28u) && !status,
            "release preserves special target");
        job.Finish();
    }

    {
        Directory compressedParent;
        if (!compressedParent.Open(deep.File(L"compressed-parent").c_str()))
        { Check(false, "prepare compressed target parent"); return; }
        auto directory = Adopt(CreateFileW(compressedParent.path.c_str(), GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr));
        USHORT format = COMPRESSION_FORMAT_DEFAULT;
        DWORD bytes = 0;
        const bool supported = directory && DeviceIoControl(directory.get(), FSCTL_SET_COMPRESSION,
            &format, sizeof(format), nullptr, 0, &bytes, nullptr);
        const DWORD setupError = supported ? ERROR_SUCCESS : GetLastError();
        directory.reset();
        if (!supported && (setupError == ERROR_NOT_SUPPORTED || setupError == ERROR_INVALID_FUNCTION))
            std::printf("SKIP: filesystem does not support compressed directories (%lu)\n", setupError);
        else if (!supported) Check(false, "set compressed target parent");
        else
        {
            JobPipe job;
            DWORD status = ERROR_GEN_FAILURE;
            ThinWireReply reply{};
            const wchar_t* leaf = L"inherited-東.txt";
            const bool sent = job.Open() && job.Thin(source, sourceLeaf, sourceId, compressedParent, leaf,
                Operation::ThinTargetCreate, nullptr, false, status, reply, input.get(),
                INVALID_HANDLE_VALUE, false, 28u);
            const FILE_ID_INFO targetId = reply.target;
            auto output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
            Check(sent && !status && output &&
                (GetFileAttributesW(compressedParent.File(leaf).c_str()) & FILE_ATTRIBUTE_COMPRESSED),
                "created target inherits compression without failing the approved create");
            output.reset();
            Check(job.Thin(source, sourceLeaf, sourceId, compressedParent, leaf, Operation::ThinTargetRetry,
                &targetId, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, 28u) &&
                !status && reply.value, "inherited compressed target permits Retry");
            auto retry = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
            retry.reset();
            Check(job.Thin(source, sourceLeaf, sourceId, compressedParent, leaf, Operation::ThinDiscardTarget,
                &targetId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, 28u) &&
                !status && IsAbsent(compressedParent.File(leaf)), "cleanup removes only created compressed target");
            job.Finish();
        }
    }

    for (int scenario = 0; scenario < 7; ++scenario)
    {
        if (scenario == 3) continue; // sparse overwrite is covered above
        const std::wstring leaf = L"negative-" + std::to_wstring(scenario) + L".txt";
        const auto path = deep.File(leaf);
        Check(WriteByte(path, 0x63) && WriteByte(path + L":data", 0x64), "prepare refusal fixture");
        ThinDenyAcl denied;
        Handle sharing;
        if (scenario == 0)
            sharing = Adopt(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
        if (scenario == 1)
            Check(denied.Apply(path, false, L"D:P(D;;0x2;;;WD)(A;;FA;;;WD)"), "deny write data but permit metadata");
        if (scenario <= 1) Check(SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN),
            "set attributes before failed native overwrite");
        if (scenario == 2)
            Check(CreateHardLinkW((path + L".alias").c_str(), path.c_str(), nullptr), "prepare hardlink boundary");
        JobPipe job;
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        Check(job.Open(nullptr, nullptr, Operation::FindContent, nullptr, false, nullptr, scenario == 4),
            "open refusal session");
        if (scenario == 5)
            Check(job.Thin(source, sourceLeaf, sourceId, outside, L"bind-scope.txt", Operation::ThinTargetProbe,
                nullptr, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, 28u) &&
                status == ERROR_FILE_NOT_FOUND, "bind independent target scope without mutation");
        auto id = sourceId;
        if (scenario == 6) id.FileId.Identifier[0] ^= 0x55;
        const bool sent = job.Thin(source, sourceLeaf, id, deep, leaf, Operation::ThinTargetOverwrite,
            nullptr, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, 28u);
        const DWORD expected = scenario == 0 ? ERROR_SHARING_VIOLATION :
            scenario == 1 || scenario == 5 ? ERROR_ACCESS_DENIED : scenario == 4 ? ERROR_PARTIAL_COPY :
            scenario == 6 ? ERROR_FILE_INVALID : ERROR_NOT_SUPPORTED;
        Check(sent && status == expected && !reply.value, "failed or unsupported overwrite delivers no handle");
        sharing.reset();
        if (scenario == 4)
        {
            ThinOverwriteSnapshot partial;
            Check(partial.Read(path) && partial.size == 0 && IsAbsent(path + L":data") &&
                UacF7::TransferReply(Operation::ThinTargetOverwrite, status, true).unknown,
                "failed handoff after truncation preserves object and reports partial result");
        }
        else
            Check(HasByte(path, 0x63) && HasByte(path + L":data", 0x64), "refusal preserves main content and ADS");
        if (scenario <= 1)
            Check((GetFileAttributesW(path.c_str()) & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN)) ==
                  (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN), "failed data open restores original attributes");
        job.Finish();
        Check(denied.Restore(), "restore only test ACL");
        SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
    }

    Directory directoryTarget;
    Check(directoryTarget.Open(deep.File(L"directory-target").c_str()) &&
        WriteByte(directoryTarget.File(L"keep.txt"), 0x38), "prepare directory type boundary");
    {
        JobPipe job;
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        Check(job.Open() && job.Thin(source, sourceLeaf, sourceId, deep, L"directory-target",
            Operation::ThinTargetOverwrite, nullptr, false, status, reply, input.get(),
            INVALID_HANDLE_VALUE, false, 28u) && status && !reply.value &&
            HasByte(directoryTarget.File(L"keep.txt"), 0x38), "file overwrite cannot mutate a directory");
        job.Finish();
    }
    const auto link = deep.File(L"symbolic-link-target.txt");
    const auto referent = outside.File(L"do-not-overwrite.txt");
    Check(WriteByte(referent, 0x39), "prepare out-of-scope symlink referent");
    if (CreateSymbolicLinkW(link.c_str(), referent.c_str(), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
    {
        JobPipe job;
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        Check(job.Open() && job.Thin(source, sourceLeaf, sourceId, deep, L"symbolic-link-target.txt",
            Operation::ThinTargetOverwrite, nullptr, false, status, reply, input.get(),
            INVALID_HANDLE_VALUE, false, 28u) && status && !reply.value && HasByte(referent, 0x39),
            "overwrite never follows a target symlink outside the approved parent");
        job.Finish();
        Check(DeleteFileW(link.c_str()), "remove only test symlink");
    }
    else
        std::printf("SKIP symlink-target fixture: Windows error %lu\n", GetLastError());

    {
        const auto path = deep.File(L"invalid-reopen-option.txt");
        Check(WriteByte(path, 0x41), "prepare invalid reopen option fixture");
        JobPipe invalid;
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        Check(invalid.Open() && !invalid.Thin(source, sourceLeaf, sourceId, deep, L"invalid-reopen-option.txt",
            Operation::ThinTargetOverwrite, nullptr, false, status, reply, input.get(),
            INVALID_HANDLE_VALUE, false, 60u) && HasByte(path, 0x41),
            "write-only reopen option cannot modify an overwrite target");
        invalid.Finish();
    }
    JobPipe missing;
    DWORD status = ERROR_GEN_FAILURE;
    ThinWireReply reply{};
    Check(missing.Open() && missing.Thin(source, sourceLeaf, sourceId, deep, L"removed-before-open.txt",
        Operation::ThinTargetOverwrite, nullptr, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, 28u) &&
        !status && reply.value, "native CREATE_ALWAYS semantics create missing target");
    auto created = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
    created.reset(); missing.Finish();

    JobPipe self;
    Check(self.Open() && self.Thin(source, sourceLeaf, sourceId, source, sourceLeaf,
        Operation::ThinTargetOverwrite, nullptr, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, 28u) &&
        status == ERROR_SHARING_VIOLATION && !reply.value, "never truncate the source object");
    self.Finish(); input.reset();
    Check(HasByte(source.File(sourceLeaf), 0x42), "all overwrite scenarios preserve source");
}
