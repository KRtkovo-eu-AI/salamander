/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-11 | Exercise read-only F6 publication, rollback and interrupted source retirement. | Preserve exact source identities and attributes across ordinary and guarded moves. | | GPT-6.1 Sol |
| 2026-11-11 | Verify read-only copies with exact protected security and publication cancellation. | Cover combined DACL preservation and cleanup through the real NTFS primitive. | | GPT-6.1 Sol |
| 2026-11-11 | Exercise streamed native copies and ordinary attributes on NTFS. | Verify large data, read-only rollback and guarded source retirement with real files. | | GPT-6.1 Sol |
*/
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define SAM_UAC_MOVE_SOURCE_FAULT_TEST
#include <windows.h>
#include <sddl.h>
#include <cstdio>
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>
#include "../uacbroker/copyleaf.h"
#include "../uacbroker/movesourceguard.h"
#include "../uacbroker/deleteleaf.h"

namespace UacMoveSourceJournal
{
DWORD crashAfterRemainingLinks = MAXDWORD;
void AfterMoveLinkClosedForTest(DWORD remainingLinks)
{
    if (remainingLinks == crashAfterRemainingLinks)
        TerminateProcess(GetCurrentProcess(), 87);
}
}

namespace
{
using Handle = std::unique_ptr<void, decltype(&CloseHandle)>;
using Local = std::unique_ptr<void, decltype(&LocalFree)>;
int failures = 0;
constexpr ULONGLONG largeSize = 20 * 1024 * 1024 + 17;
constexpr ULONGLONG namedSize = 1024 * 1024 + 13;
constexpr DWORD ordinaryAttributes = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;

bool Check(bool passed, const char* text, DWORD error = 0)
{
    std::printf("%s: %s (%lu)\n", passed ? "PASS" : "FAIL", text, error);
    if (!passed) ++failures;
    return passed;
}
Handle Open(const std::wstring& path, DWORD access, DWORD creation = OPEN_EXISTING, DWORD flags = 0,
            SECURITY_ATTRIBUTES* security = nullptr)
{
    const HANDLE raw = CreateFileW(path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   security, creation, flags | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    return Handle(raw == INVALID_HANDLE_VALUE ? nullptr : raw, CloseHandle);
}
Handle Directory(const std::wstring& path)
{
    return Open(path, FILE_LIST_DIRECTORY | FILE_ADD_FILE | FILE_ADD_SUBDIRECTORY | READ_CONTROL,
                OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS);
}
bool Present(const std::wstring& path) { return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES; }
bool NoStage(const std::wstring& path)
{
    WIN32_FIND_DATAW data{};
    const HANDLE find = FindFirstFileW((path + L"\\.samandarin-copy-*.tmp").c_str(), &data);
    if (find != INVALID_HANDLE_VALUE) { FindClose(find); return false; }
    return GetLastError() == ERROR_FILE_NOT_FOUND;
}
bool WritePattern(const std::wstring& path, ULONGLONG length, BYTE value, SECURITY_ATTRIBUTES* security = nullptr)
{
    const auto file = Open(path, GENERIC_WRITE, CREATE_NEW, 0, security);
    if (!file) return false;
    std::array<BYTE, 64 * 1024> block{};
    block.fill(value);
    while (length)
    {
        const DWORD count = static_cast<DWORD>(std::min<ULONGLONG>(length, block.size()));
        DWORD written = 0;
        if (!WriteFile(file.get(), block.data(), count, &written, nullptr) || written != count) return false;
        length -= count;
    }
    return true;
}
bool PatternMatches(const std::wstring& path, ULONGLONG length, BYTE value)
{
    const auto file = Open(path, GENERIC_READ);
    LARGE_INTEGER size{};
    if (!file || !GetFileSizeEx(file.get(), &size) || static_cast<ULONGLONG>(size.QuadPart) != length) return false;
    std::array<BYTE, 64 * 1024> block{};
    while (length)
    {
        const DWORD count = static_cast<DWORD>(std::min<ULONGLONG>(length, block.size()));
        DWORD read = 0;
        if (!ReadFile(file.get(), block.data(), count, &read, nullptr) || read != count ||
            !std::all_of(block.begin(), block.begin() + count, [value](BYTE byte) { return byte == value; })) return false;
        length -= count;
    }
    return true;
}
bool Identity(const std::wstring& path, FILE_ID_INFO& id)
{
    const auto file = Open(path, FILE_READ_ATTRIBUTES);
    return file && !UacRenameLeaf::Detail::ReadSourceIdentity(file.get(), id);
}
bool AttributesMatch(const std::wstring& path, DWORD expected)
{
    return GetFileAttributesW(path.c_str()) == expected;
}
bool AllowFixture(HANDLE, bool) { return true; }
bool SelectionCases()
{
    using namespace UacF7;
    const DWORD compressed = ordinaryAttributes | FILE_ATTRIBUTE_COMPRESSED;
    const DWORD sparse = ordinaryAttributes | FILE_ATTRIBUTE_SPARSE_FILE;
    Check(FileTransferOperation(compressed, false, true, false, true) == Operation::CopyCompressedSecurity &&
          IsEligibleFileCopy(compressed, true, false, true) &&
          IsEligibleTransferSource(compressed, Operation::CopyCompressedSecurity),
          "keep compressed security-copy preparation consistent for a listed protected source");
    Check(FileTransferOperation(sparse, false, false, false, false) == Operation::Copy &&
          IsEligibleFileCopy(sparse, false, false, false) && IsEligibleTransferSource(sparse, Operation::Copy),
          "prepare sparse read-only copying with the same operation in panel and worker");
    Check(FileTransferOperation(ordinaryAttributes, false, false, true, false) == Operation::CopyIgnoreAds &&
          IsEligibleFileCopy(ordinaryAttributes, false, true, false) &&
          IsEligibleTransferSource(ordinaryAttributes, Operation::CopyIgnoreAds),
          "prepare explicit named-stream omission without dropping ordinary attributes");
    Check(!IsEligibleFileCopy(compressed, true, true, false) &&
          !IsEligibleFileCopy(ordinaryAttributes | FILE_ATTRIBUTE_ENCRYPTED, false, false, false) &&
          IsEligibleMoveFile(ordinaryAttributes, false, false) &&
          IsEligibleTransferSource(ordinaryAttributes, Operation::Move) &&
          !IsEligibleTransferSource(FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_ARCHIVE, Operation::Copy) &&
          NativeCopyByteLimit(Operation::Copy) > (1ULL << 32) &&
          NativeCopyByteLimit(Operation::CopyEncrypted) == maxPilotCopyBytes &&
          NativeCopyByteLimit(Operation::WriteZipSelection) == maxPilotCopyBytes,
          "admit ordinary read-only moves while retaining unsupported storage combinations and snapshot budgets");
    return failures == 0;
}
struct Progress
{
    ULONGLONG bytes = 0;
    bool cancel = false;
    static DWORD Update(void* context, ULONGLONG count, ULONGLONG)
    {
        auto& self = *static_cast<Progress*>(context);
        self.bytes += count;
        return self.cancel ? ERROR_CANCELLED : ERROR_SUCCESS;
    }
};
struct CopyFixture
{
    std::wstring root;
    std::wstring source;
    std::wstring target;
    Handle from{nullptr, CloseHandle}, to{nullptr, CloseHandle};
    UacDirectoryLeaf::ParentIdentity fromId, toId;
    FILE_ID_INFO fileId{};
    explicit CopyFixture(const std::wstring& directory) : root(directory), source(root + L"\\source-東"), target(root + L"\\target-日") {}
    bool OpenParents()
    {
        from = Directory(source); to = Directory(target);
        return from && to && !UacDirectoryLeaf::ReadParentIdentity(from.get(), fromId) &&
            !UacDirectoryLeaf::ReadParentIdentity(to.get(), toId) && Identity(source + L"\\large-東.bin", fileId);
    }
    DWORD Copy(std::wstring_view leaf, const UacCopyLeaf::Detail::CopyProgress* progress = nullptr,
               const UacCopyLeaf::Detail::PublicationGate* gate = nullptr, const FILE_ID_INFO* id = nullptr)
    {
        return UacCopyLeaf::CopyVerifiedFile(from.get(), fromId, id ? *id : fileId, L"large-東.bin", to.get(), toId,
            leaf, nullptr, nullptr, MAXULONGLONG, largeSize, UacCopyLeaf::StreamPolicy::PreserveNamed,
            AllowFixture, false, false, progress, gate);
    }
};

bool CopyCases(CopyFixture& fixture)
{
    if (!CreateDirectoryW(fixture.source.c_str(), nullptr) || !CreateDirectoryW(fixture.target.c_str(), nullptr) ||
        !WritePattern(fixture.source + L"\\large-東.bin", largeSize, 0x53) ||
        !WritePattern(fixture.source + L"\\large-東.bin:stream-日", namedSize, 0xa7) ||
        !SetFileAttributesW((fixture.source + L"\\large-東.bin").c_str(), ordinaryAttributes) || !fixture.OpenParents()) return false;
    Progress progress;
    const UacCopyLeaf::Detail::CopyProgress notify{Progress::Update, &progress};
    DWORD error = fixture.Copy(L"large-copy.bin", &notify);
    if (!Check(!error, "copy a read-only, hidden, system file beyond 16 MiB with its named stream", error)) return false;
    Check(PatternMatches(fixture.target + L"\\large-copy.bin", largeSize, 0x53) &&
          PatternMatches(fixture.target + L"\\large-copy.bin:stream-日", namedSize, 0xa7) &&
          progress.bytes == largeSize + namedSize, "verify every copied byte and progress across main and named streams");
    FILE_ID_INFO unchanged{};
    Check(Identity(fixture.source + L"\\large-東.bin", unchanged) &&
          UacRenameLeaf::Detail::SameSource(unchanged, fixture.fileId) &&
          AttributesMatch(fixture.source + L"\\large-東.bin", ordinaryAttributes) &&
          AttributesMatch(fixture.target + L"\\large-copy.bin", ordinaryAttributes | FILE_ATTRIBUTE_ARCHIVE),
          "preserve source identity and attributes and apply ordinary copy attributes to the destination");
    error = fixture.Copy(L"large-copy.bin");
    Check((error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS) && NoStage(fixture.target) &&
          PatternMatches(fixture.target + L"\\large-copy.bin", largeSize, 0x53),
          "a publication collision cleans the read-only stage and keeps the existing destination", error);
    auto wrong = fixture.fileId; wrong.FileId.Identifier[0] ^= 1;
    error = fixture.Copy(L"wrong-identity.bin", nullptr, nullptr, &wrong);
    Check(error == ERROR_FILE_INVALID && !Present(fixture.target + L"\\wrong-identity.bin") && NoStage(fixture.target),
          "reject a substituted source identity before publication", error);
    Progress cancelled; cancelled.cancel = true;
    const UacCopyLeaf::Detail::CopyProgress stop{Progress::Update, &cancelled};
    error = fixture.Copy(L"cancelled.bin", &stop);
    Check(error == ERROR_CANCELLED && cancelled.bytes && cancelled.bytes < largeSize &&
          !Present(fixture.target + L"\\cancelled.bin") && NoStage(fixture.target),
          "cancel after transferred bytes and remove the incomplete stage", error);
    const UacCopyLeaf::Detail::PublicationGate reject{
        [](void*, HANDLE, ULONGLONG) -> DWORD { return ERROR_CANCELLED; }, nullptr};
    error = fixture.Copy(L"late-cancel.bin", nullptr, &reject);
    Check(error == ERROR_CANCELLED && !Present(fixture.target + L"\\late-cancel.bin") && NoStage(fixture.target),
          "cancel after read-only metadata is applied and remove the complete stage", error);
    return true;
}

bool SparseCase(CopyFixture& fixture)
{
    constexpr ULONGLONG size = (1ULL << 32) + 65537;
    const auto path = fixture.source + L"\\sparse-東.bin";
    {
        const auto file = Open(path, GENERIC_READ | GENERIC_WRITE, CREATE_NEW);
        DWORD bytes = 0;
        const BYTE marker = 0x65;
        LARGE_INTEGER position{};
        if (!file || !DeviceIoControl(file.get(), FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &bytes, nullptr)) return false;
        for (const ULONGLONG offset : {0ULL, size - 1})
        {
            position.QuadPart = static_cast<LONGLONG>(offset);
            if (!SetFilePointerEx(file.get(), position, nullptr, FILE_BEGIN) ||
                !WriteFile(file.get(), &marker, 1, &bytes, nullptr) || bytes != 1) return false;
        }
    }
    FILE_ID_INFO id{};
    if (!Identity(path, id)) return false;
    const DWORD error = UacCopyLeaf::CopyVerifiedFile(fixture.from.get(), fixture.fromId, id, L"sparse-東.bin",
        fixture.to.get(), fixture.toId, L"sparse-copy.bin", nullptr, nullptr, MAXULONGLONG, size,
        UacCopyLeaf::StreamPolicy::PreserveNamed, AllowFixture);
    if (!Check(!error, "stream a sparse native copy across the 4 GiB offset boundary", error)) return false;
    const auto output = Open(fixture.target + L"\\sparse-copy.bin", GENERIC_READ);
    LARGE_INTEGER length{};
    bool valid = output && GetFileSizeEx(output.get(), &length) && static_cast<ULONGLONG>(length.QuadPart) == size;
    for (const ULONGLONG offset : {0ULL, 1ULL << 31, size - 1})
    {
        LARGE_INTEGER position{}; position.QuadPart = static_cast<LONGLONG>(offset);
        BYTE value = 0xff; DWORD read = 0;
        valid = valid && SetFilePointerEx(output.get(), position, nullptr, FILE_BEGIN) &&
            ReadFile(output.get(), &value, 1, &read, nullptr) && read == 1 && value == (offset == (1ULL << 31) ? 0 : 0x65);
    }
    FILE_ALLOCATED_RANGE_BUFFER query{}, range{};
    query.FileOffset.QuadPart = 1LL << 30; query.Length.QuadPart = 1LL << 30;
    DWORD returned = 0;
    valid = valid && DeviceIoControl(output.get(), FSCTL_QUERY_ALLOCATED_RANGES, &query, sizeof(query),
        &range, sizeof(range), &returned, nullptr) && returned == 0;
    Check(valid, "preserve logical size, data beyond 4 GiB and the unallocated middle range");
    return true;
}

bool CompressedCase(CopyFixture& fixture)
{
    const auto path = fixture.source + L"\\compressed-東.bin";
    if (!WritePattern(path, 262147, 0x72)) return false;
    {
        const auto file = Open(path, GENERIC_READ | GENERIC_WRITE);
        if (!file || UacCopyLeaf::Detail::SetCompression(file.get(), COMPRESSION_FORMAT_LZNT1)) return false;
    }
    if (!SetFileAttributesW(path.c_str(), ordinaryAttributes)) return false;
    FILE_ID_INFO id{};
    if (!Identity(path, id)) return false;
    const DWORD error = UacCopyLeaf::CopyVerifiedFile(fixture.from.get(), fixture.fromId, id, L"compressed-東.bin",
        fixture.to.get(), fixture.toId, L"compressed-copy.bin", nullptr, nullptr, MAXULONGLONG, 262147,
        UacCopyLeaf::StreamPolicy::PreserveNamed, AllowFixture, true);
    USHORT format = COMPRESSION_FORMAT_NONE;
    const auto output = Open(fixture.target + L"\\compressed-copy.bin", GENERIC_READ);
    Check(!error && output && !UacCopyLeaf::Detail::ReadCompression(output.get(), format) &&
          format == COMPRESSION_FORMAT_LZNT1 &&
          AttributesMatch(fixture.target + L"\\compressed-copy.bin", ordinaryAttributes | FILE_ATTRIBUTE_COMPRESSED) &&
          PatternMatches(fixture.target + L"\\compressed-copy.bin", 262147, 0x72),
          "preserve compression and exact read-only/hidden/system attributes at publication", error);
    return !error;
}

bool SameVolumeMoveCase(CopyFixture& fixture)
{
    const auto source = fixture.source + L"\\same-volume.bin";
    const auto target = fixture.target + L"\\same-volume.bin";
    constexpr DWORD attributes = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE;
    if (!WritePattern(source, 37, 0x32) || !SetFileAttributesW(source.c_str(), attributes)) return false;
    FILE_ID_INFO id{}, moved{};
    if (!Identity(source, id)) return false;
    const DWORD error = UacRenameLeaf::MoveVerifiedFile(fixture.from.get(), fixture.fromId, id, L"same-volume.bin",
        fixture.to.get(), fixture.toId, L"same-volume.bin");
    Check(!error && !Present(source) && Identity(target, moved) && UacRenameLeaf::Detail::SameSource(id, moved) &&
          AttributesMatch(target, attributes) && PatternMatches(target, 37, 0x32),
          "move a hidden/system native file on the same volume without changing identity or contents", error);
    return !error;
}

Local Security(const std::wstring& sddl)
{
    PSECURITY_DESCRIPTOR raw = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &raw, nullptr)) return Local(nullptr, LocalFree);
    return Local(raw, LocalFree);
}
std::wstring CurrentUserSid()
{
    HANDLE tokenRaw = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenRaw)) return {};
    const Handle token(tokenRaw, CloseHandle);
    DWORD size = 0; GetTokenInformation(token.get(), TokenUser, nullptr, 0, &size);
    std::vector<BYTE> data(size);
    if (!size || !GetTokenInformation(token.get(), TokenUser, data.data(), size, &size)) return {};
    PWSTR sid = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(data.data())->User.Sid, &sid)) return {};
    const Local sidMemory(sid, LocalFree);
    return sid;
}

std::wstring ProtectedSecurity(const std::wstring& path)
{
    const auto file = Open(path, READ_CONTROL);
    PSECURITY_DESCRIPTOR raw = nullptr;
    constexpr SECURITY_INFORMATION requested = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
    if (!file || GetSecurityInfo(file.get(), SE_FILE_OBJECT, requested, nullptr, nullptr, nullptr, nullptr, &raw)) return {};
    const Local descriptor(raw, LocalFree);
    SECURITY_DESCRIPTOR_CONTROL control{};
    DWORD revision = 0;
    if (!GetSecurityDescriptorControl(raw, &control, &revision) || !(control & SE_DACL_PROTECTED)) return {};
    PWSTR text = nullptr;
    if (!ConvertSecurityDescriptorToStringSecurityDescriptorW(raw, SDDL_REVISION_1, requested, &text, nullptr)) return {};
    const Local memory(text, LocalFree);
    return text;
}

bool SameProtectedSecurity(const std::wstring& first, const std::wstring& second)
{
    // SetSecurityInfo may add the AUTO_INHERITED bookkeeping bit while
    // restoring a protected DACL. Compare its actual policy and principals.
    const auto left = Open(first, READ_CONTROL), right = Open(second, READ_CONTROL);
    PSECURITY_DESCRIPTOR a = nullptr, b = nullptr;
    PSID ownerA = nullptr, ownerB = nullptr, groupA = nullptr, groupB = nullptr;
    PACL daclA = nullptr, daclB = nullptr;
    constexpr SECURITY_INFORMATION requested = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
    const DWORD errorA = left ? GetSecurityInfo(left.get(), SE_FILE_OBJECT, requested, &ownerA, &groupA, &daclA, nullptr, &a) : ERROR_INVALID_HANDLE;
    const Local memoryA(a, LocalFree);
    const DWORD errorB = right ? GetSecurityInfo(right.get(), SE_FILE_OBJECT, requested, &ownerB, &groupB, &daclB, nullptr, &b) : ERROR_INVALID_HANDLE;
    const Local memoryB(b, LocalFree);
    SECURITY_DESCRIPTOR_CONTROL controlA{}, controlB{}; DWORD revision = 0;
    return !errorA && !errorB && ownerA && ownerB && groupA && groupB && daclA && daclB &&
        GetSecurityDescriptorControl(a, &controlA, &revision) && GetSecurityDescriptorControl(b, &controlB, &revision) &&
        (controlA & SE_DACL_PROTECTED) && (controlB & SE_DACL_PROTECTED) &&
        EqualSid(ownerA, ownerB) && EqualSid(groupA, groupB) && daclA->AclSize == daclB->AclSize &&
        std::memcmp(daclA, daclB, daclA->AclSize) == 0;
}

bool ReadOnlySecurityCase(CopyFixture& fixture)
{
    const auto sid = CurrentUserSid();
    const auto security = Security(std::wstring(L"O:") + sid + L"D:P(A;;FA;;;" + sid + L")");
    if (sid.empty() || !security) return false;
    SECURITY_ATTRIBUTES attributes{sizeof(attributes), security.get(), FALSE};
    const auto source = fixture.source + L"\\secure-readonly-東.bin";
    const auto target = fixture.target + L"\\secure-readonly-copy.bin";
    if (!WritePattern(source, 100003, 0x37, &attributes) || !WritePattern(source + L":named-日", 67, 0x49) ||
        !SetFileAttributesW(source.c_str(), ordinaryAttributes)) return false;
    FILE_ID_INFO id{};
    if (!Identity(source, id)) return false;
    const auto originalSecurity = ProtectedSecurity(source);
    if (originalSecurity.empty()) return false;
    const auto copy = [&](std::wstring_view leaf, const UacCopyLeaf::Detail::PublicationGate* gate = nullptr) {
        return UacCopyLeaf::CopyVerifiedFile(fixture.from.get(), fixture.fromId, id, L"secure-readonly-東.bin",
            fixture.to.get(), fixture.toId, leaf, nullptr, nullptr, MAXULONGLONG, 100003,
            UacCopyLeaf::StreamPolicy::PreserveNamed, AllowFixture, false, true, nullptr, gate);
    };
    DWORD error = copy(L"secure-readonly-copy.bin");
    if (!Check(!error && ProtectedSecurity(target) == originalSecurity && ProtectedSecurity(source) == originalSecurity &&
          AttributesMatch(target, ordinaryAttributes | FILE_ATTRIBUTE_ARCHIVE) && AttributesMatch(source, ordinaryAttributes) &&
          PatternMatches(target, 100003, 0x37) && PatternMatches(target + L":named-日", 67, 0x49),
          "copy read-only data and named streams with independently verified owner, group and protected DACL", error)) return false;
    bool reached = false;
    const UacCopyLeaf::Detail::PublicationGate reject{
        [](void* context, HANDLE, ULONGLONG) -> DWORD { *static_cast<bool*>(context) = true; return ERROR_CANCELLED; }, &reached};
    error = copy(L"secure-readonly-cancel.bin", &reject);
    FILE_ID_INFO current{};
    Check(error == ERROR_CANCELLED && reached && !Present(fixture.target + L"\\secure-readonly-cancel.bin") &&
          NoStage(fixture.target) && Identity(source, current) && UacRenameLeaf::Detail::SameSource(id, current) &&
          ProtectedSecurity(source) == originalSecurity && AttributesMatch(source, ordinaryAttributes) &&
          PatternMatches(source, 100003, 0x37) && PatternMatches(source + L":named-日", 67, 0x49),
          "cancel a secured read-only copy at publication and clean staging without changing source identity or security", error);
    return error == ERROR_CANCELLED;
}

bool MoveCase(CopyFixture& fixture)
{
    const auto sid = CurrentUserSid();
    if (sid.empty()) return false;
    const auto security = Security(std::wstring(L"O:") + sid + L"D:P(A;;FA;;;" + sid + L")");
    const auto sealSecurity = Security(std::wstring(L"D:P(A;;RC;;;OW)(A;;0x00170189;;;") + sid + L")");
    PACL seal = nullptr; BOOL present = FALSE, defaulted = FALSE;
    if (!security || !sealSecurity || !GetSecurityDescriptorDacl(sealSecurity.get(), &present, &seal, &defaulted) || !present) return false;
    SECURITY_ATTRIBUTES attributes{sizeof(attributes), security.get(), FALSE};
    const auto storagePath = fixture.root + L"\\move-private";
    if (!CreateDirectoryW(storagePath.c_str(), &attributes)) return false;
    const auto storage = Directory(storagePath);
    const auto journal = Open(storagePath + L"\\journal", GENERIC_READ | GENERIC_WRITE, CREATE_NEW, 0, &attributes);
    const auto sourcePath = fixture.source + L"\\move-hidden-system.bin";
    constexpr DWORD moveAttributes = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE;
    if (!storage || !journal || !WritePattern(sourcePath, 131079, 0x29) ||
        !SetFileAttributesW(sourcePath.c_str(), moveAttributes)) return false;
    FILE_ID_INFO id{};
    if (!Identity(sourcePath, id)) return false;
    HANDLE raw = nullptr;
    DWORD error = UacRenameLeaf::Detail::OpenSource(fixture.from.get(), L"move-hidden-system.bin", raw,
                                                   DELETE | READ_CONTROL | WRITE_DAC, FILE_SHARE_READ);
    Handle source(raw, CloseHandle);
    std::wstring alias;
    if (!error) error = UacMoveSourceJournal::Arm(fixture.from.get(), L"move-hidden-system.bin", source.get(), id,
        storage.get(), journal.get(), security.get(), alias, seal);
    source.reset();
    if (!Check(!error, "seal hidden/system move source and record its recovery identity", error)) return false;
    {
        UacMoveSourceJournal::SourceGuard guard;
        error = guard.Open(fixture.from.get(), L"move-hidden-system.bin", id, 2);
        Handle target(nullptr, CloseHandle); FILE_ID_INFO targetId{};
        if (!error) error = guard.PublishCopy(fixture.to.get(), fixture.toId, L"moved-hidden-system.bin", 131079,
            nullptr, nullptr, MAXULONGLONG, target, targetId);
        if (!Check(!error, "publish hidden/system bytes while retaining the guarded source", error)) return false;
        error = guard.RetirePublishedSource(fixture.from.get(), storage.get(), journal.get(), security.get(),
            fixture.to.get(), L"moved-hidden-system.bin", target.get(), targetId, nullptr, nullptr, MAXULONGLONG);
    }
    if (!Check(!error, "retire hidden/system source only after publication and receipt", error)) return false;
    error = UacMoveSourceJournal::Recover(fixture.from.get(), storage.get(), journal.get(), security.get());
    Check(!error && !Present(sourcePath) && !Present(storagePath + L"\\" + alias) &&
        AttributesMatch(fixture.target + L"\\moved-hidden-system.bin", moveAttributes) &&
        PatternMatches(fixture.target + L"\\moved-hidden-system.bin", 131079, 0x29),
        "recover the completed move receipt without changing hidden/system destination data", error);
    return !error;
}

struct ReadOnlyMoveFixture
{
    static constexpr wchar_t leaf[] = L"read-only-東.bin";
    static constexpr ULONGLONG size = 131079;
    std::wstring base, source, target, storagePath, alias;
    Local security{nullptr, LocalFree}, sealSecurity{nullptr, LocalFree};
    Handle from{nullptr, CloseHandle}, to{nullptr, CloseHandle}, storage{nullptr, CloseHandle}, journal{nullptr, CloseHandle};
    UacDirectoryLeaf::ParentIdentity fromId, toId;
    FILE_ID_INFO id{};
    PACL seal = nullptr;
    DWORD attributes = ordinaryAttributes;

    explicit ReadOnlyMoveFixture(const std::wstring& root, const std::wstring& name) : base(root + L"\\readonly-" + name) {}
    std::wstring Source() const { return source + L"\\" + leaf; }
    std::wstring Target() const { return target + L"\\" + leaf; }
    bool Init(bool create, bool compressed = false)
    {
        const auto sid = CurrentUserSid();
        security = Security(std::wstring(L"O:") + sid + L"D:P(A;;FA;;;" + sid + L")");
        sealSecurity = Security(std::wstring(L"D:P(A;;RC;;;OW)(A;;0x00170189;;;") + sid + L")");
        BOOL present = FALSE, defaulted = FALSE;
        if (sid.empty() || !security || !sealSecurity ||
            !GetSecurityDescriptorDacl(sealSecurity.get(), &present, &seal, &defaulted) || !present) return false;
        SECURITY_ATTRIBUTES descriptor{sizeof(descriptor), security.get(), FALSE};
        if (create && !CreateDirectoryW(base.c_str(), &descriptor)) return false;
        auto common = base;
        for (int i = 0; i < 2; ++i)
        {
            common += L"\\階層-" + std::wstring(96, L'日');
            if (create && !CreateDirectoryW(common.c_str(), nullptr)) return false;
        }
        source = common + L"\\source-東";
        target = common + L"\\target-日";
        storagePath = base + L"\\private";
        if (create && (!CreateDirectoryW(source.c_str(), nullptr) || !CreateDirectoryW(target.c_str(), nullptr) ||
                       !CreateDirectoryW(storagePath.c_str(), &descriptor))) return false;
        from = Directory(source); to = Directory(target); storage = Directory(storagePath);
        journal = Open(storagePath + L"\\journal", GENERIC_READ | GENERIC_WRITE, create ? CREATE_NEW : OPEN_EXISTING, 0, &descriptor);
        if (!from || !to || !storage || !journal || UacDirectoryLeaf::ReadParentIdentity(from.get(), fromId) ||
            UacDirectoryLeaf::ReadParentIdentity(to.get(), toId)) return false;
        if (create)
        {
            if (!WritePattern(Source(), size, 0x63, &descriptor) || !WritePattern(Source() + L":named-日", 131, 0xa8) ||
                !WritePattern(base + L"\\reference", 1, 0x17, &descriptor)) return false;
            if (compressed)
            {
                const auto file = Open(Source(), GENERIC_READ | GENERIC_WRITE);
                if (!file || UacCopyLeaf::Detail::SetCompression(file.get(), COMPRESSION_FORMAT_LZNT1)) return false;
                attributes |= FILE_ATTRIBUTE_COMPRESSED;
            }
            if (attributes & FILE_ATTRIBUTE_SPARSE_FILE)
            {
                const auto file = Open(Source(), GENERIC_READ | GENERIC_WRITE);
                DWORD returned = 0;
                if (!file || !DeviceIoControl(file.get(), FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &returned, nullptr)) return false;
            }
            if (!SetFileAttributesW(Source().c_str(), attributes & ~(FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE)) ||
                !Identity(Source(), id)) return false;
            const auto expected = Open(base + L"\\identity", GENERIC_WRITE, CREATE_NEW);
            DWORD written = 0;
            if (!expected || !WriteFile(expected.get(), &id, sizeof(id), &written, nullptr) || written != sizeof(id)) return false;
        }
        else
        {
            const auto expected = Open(base + L"\\identity", GENERIC_READ);
            DWORD read = 0;
            if (!expected || !ReadFile(expected.get(), &id, sizeof(id), &read, nullptr) || read != sizeof(id)) return false;
        }
        return true;
    }
    DWORD Arm()
    {
        HANDLE raw = nullptr;
        DWORD error = UacRenameLeaf::Detail::OpenSource(from.get(), leaf, raw,
            DELETE | READ_CONTROL | WRITE_DAC | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ);
        const Handle file(raw, CloseHandle);
        return error ? error : UacMoveSourceJournal::Arm(from.get(), leaf, file.get(), id,
            storage.get(), journal.get(), security.get(), alias, seal);
    }
    DWORD Recover() { return UacMoveSourceJournal::Recover(from.get(), storage.get(), journal.get(), security.get()); }
    bool SourceIntact()
    {
        FILE_ID_INFO current{};
        return Identity(Source(), current) && UacRenameLeaf::Detail::SameSource(id, current) &&
            AttributesMatch(Source(), attributes) && PatternMatches(Source(), size, 0x63) &&
            PatternMatches(Source() + L":named-日", 131, 0xa8) && SameProtectedSecurity(Source(), base + L"\\reference");
    }
    bool TargetMatches()
    {
        return AttributesMatch(Target(), attributes) && PatternMatches(Target(), size, 0x63) &&
            PatternMatches(Target() + L":named-日", 131, 0xa8);
    }
};

bool ReadOnlySameVolume(const std::wstring& root)
{
    ReadOnlyMoveFixture fixture(root, L"same-volume");
    if (!fixture.Init(true) || !WritePattern(fixture.Target(), 1, 0x11)) return false;
    const auto move = [&](const FILE_ID_INFO& expected, ULONGLONG deadline = MAXULONGLONG) {
        return UacRenameLeaf::MoveVerifiedFile(fixture.from.get(), fixture.fromId, expected, fixture.leaf,
            fixture.to.get(), fixture.toId, fixture.leaf, UacRenameLeaf::MoveKind::OrdinaryFile, nullptr, deadline);
    };
    DWORD error = move(fixture.id);
    Check((error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS) && fixture.SourceIntact() &&
        PatternMatches(fixture.Target(), 1, 0x11), "read-only same-volume collision preserves both files and all source attributes", error);
    if (!DeleteFileW(fixture.Target().c_str())) return false;
    error = move(fixture.id, 0);
    Check(error == ERROR_TIMEOUT && fixture.SourceIntact() && !Present(fixture.Target()),
          "cancel same-volume read-only move before mutation on a long Unicode path", error);
    auto wrong = fixture.id; wrong.FileId.Identifier[0] ^= 1;
    error = move(wrong);
    Check(error == ERROR_FILE_INVALID && fixture.SourceIntact(), "refuse a stale read-only same-volume source identity", error);
    error = move(fixture.id);
    FILE_ID_INFO current{};
    Check(!error && !Present(fixture.Source()) && Identity(fixture.Target(), current) &&
          UacRenameLeaf::Detail::SameSource(fixture.id, current) &&
          AttributesMatch(fixture.Target(), fixture.attributes | FILE_ATTRIBUTE_ARCHIVE) &&
          PatternMatches(fixture.Target(), fixture.size, 0x63) && PatternMatches(fixture.Target() + L":named-日", 131, 0xa8),
          "move a read-only file and ADS on a long Unicode path while preserving NTFS identity", error);
    return !error;
}

bool GuardedAttributeMove(const std::wstring& root, const std::wstring& name, bool compressed = false,
                         DWORD attributes = ordinaryAttributes)
{
    ReadOnlyMoveFixture fixture(root, name);
    fixture.attributes = attributes;
    if (!fixture.Init(true, compressed)) return false;
    DWORD error = fixture.Arm();
    if (error) return Check(false, "arm read-only source recovery", error);
    {
        UacMoveSourceJournal::SourceGuard guard(fixture.attributes, AllowFixture);
        error = guard.Open(fixture.from.get(), fixture.leaf, fixture.id, 2);
        if (!error) error = guard.LockStreams(fixture.storage.get(), fixture.alias);
        if (error) return Check(false, "lock read-only source streams", error);
        Handle target(nullptr, CloseHandle); FILE_ID_INFO targetId{};
        Progress cancelled; cancelled.cancel = true;
        const UacCopyLeaf::Detail::CopyProgress stop{Progress::Update, &cancelled};
        error = guard.PublishCopy(fixture.to.get(), fixture.toId, fixture.leaf, fixture.size,
            nullptr, nullptr, MAXULONGLONG, target, targetId, &stop);
        Check(error == ERROR_CANCELLED && cancelled.bytes && !target && !Present(fixture.Target()) && NoStage(fixture.target),
              "cancel a guarded move after writing bytes without publishing a destination", error);
    }
    error = fixture.Recover();
    Check(!error && fixture.SourceIntact(), "rollback restores exact attributes, streams, identity and original DACL", error);
    if (error || (error = fixture.Arm())) return Check(false, "re-arm recovered read-only move", error);
    {
        UacMoveSourceJournal::SourceGuard guard(fixture.attributes, AllowFixture);
        error = guard.Open(fixture.from.get(), fixture.leaf, fixture.id, 2);
        if (!error) error = guard.LockStreams(fixture.storage.get(), fixture.alias);
        Handle target(nullptr, CloseHandle); FILE_ID_INFO targetId{};
        if (!error) error = guard.PublishCopy(fixture.to.get(), fixture.toId, fixture.leaf, fixture.size,
            nullptr, nullptr, MAXULONGLONG, target, targetId);
        if (error) return Check(false, "publish read-only guarded destination", error);
        const Handle cancel(CreateEventW(nullptr, TRUE, TRUE, nullptr), CloseHandle);
        error = guard.RetirePublishedSource(fixture.from.get(), fixture.storage.get(), fixture.journal.get(), fixture.security.get(),
            fixture.to.get(), fixture.leaf, target.get(), targetId, cancel.get(), nullptr, MAXULONGLONG);
        Check(error == ERROR_CANCELLED, "late cancellation refuses source retirement after destination publication", error);
        error = guard.RetirePublishedSource(fixture.from.get(), fixture.storage.get(), fixture.journal.get(), fixture.security.get(),
            fixture.to.get(), fixture.leaf, target.get(), targetId, nullptr, nullptr, MAXULONGLONG);
    }
    if (error) return Check(false, "retire both read-only source links after verified publication", error);
    error = fixture.Recover();
    const DWORD expected = fixture.attributes;
    Check(!error && !Present(fixture.Source()) && !Present(fixture.storagePath + L"\\" + fixture.alias) &&
          AttributesMatch(fixture.Target(), expected) && PatternMatches(fixture.Target(), fixture.size, 0x63) &&
          PatternMatches(fixture.Target() + L":named-日", 131, 0xa8),
          compressed ? "complete read-only compressed F6 while preserving exact attributes and ADS" :
                       "complete F6 with exact attributes and ADS only after durable target publication", error);
    return !error;
}

bool ReadOnlyReceiptRetirement(const std::wstring& root)
{
    ReadOnlyMoveFixture fixture(root, L"receipt");
    FILE_ID_INFO targetId{};
    if (!fixture.Init(true) || !WritePattern(fixture.Target(), fixture.size, 0x63) ||
        !WritePattern(fixture.Target() + L":named-日", 131, 0xa8) ||
        !SetFileAttributesW(fixture.Target().c_str(), fixture.attributes) || !Identity(fixture.Target(), targetId)) return false;
    auto wrong = targetId; wrong.FileId.Identifier[0] ^= 1;
    DWORD error = UacDeleteLeaf::DeleteCopiedSource(fixture.Source(), fixture.Target(), fixture.id, wrong);
    Check(error == ERROR_FILE_INVALID && fixture.SourceIntact() && fixture.TargetMatches(),
          "refuse read-only source retirement for a stale target receipt", error);
    wrong = fixture.id; wrong.FileId.Identifier[0] ^= 1;
    error = UacDeleteLeaf::DeleteCopiedSource(fixture.Source(), fixture.Target(), wrong, targetId);
    Check(error == ERROR_FILE_INVALID && fixture.SourceIntact() && fixture.TargetMatches(),
          "refuse read-only source retirement for a replaced source without clearing its attributes", error);
    error = UacDeleteLeaf::DeleteCopiedSource(fixture.Source(), fixture.Target(), fixture.id, targetId);
    Check(!error && !Present(fixture.Source()) && fixture.TargetMatches(),
          "retire the exact read-only source while its receipt destination and ADS remain intact", error);
    return !error;
}

bool MinimalMovePreparation(const std::wstring& root)
{
    ReadOnlyMoveFixture fixture(root, L"minimal-rights");
    if (!fixture.Init(true)) return false;
    const auto sid = CurrentUserSid();
    const auto deniedMetadata = Security(std::wstring(L"D:P(D;;0x100;;;") + sid + L")(A;;FA;;;" + sid + L")");
    PACL dacl = nullptr; BOOL present = FALSE, defaulted = FALSE;
    if (!deniedMetadata || !GetSecurityDescriptorDacl(deniedMetadata.get(), &present, &dacl, &defaulted) || !present) return false;
    PACL cleanupDacl = nullptr;
    if (!GetSecurityDescriptorDacl(fixture.security.get(), &present, &cleanupDacl, &defaulted) || !present) return false;
    struct RestoreFixture
    {
        std::wstring path;
        PACL dacl;
        ~RestoreFixture()
        {
            const auto file = Open(path, WRITE_DAC | READ_CONTROL);
            if (file) SetSecurityInfo(file.get(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                nullptr, nullptr, dacl, nullptr);
        }
    } restore{fixture.Source(), cleanupDacl};
    for (const auto& path : {fixture.Source(), fixture.base + L"\\reference"})
    {
        const auto file = Open(path, WRITE_DAC | READ_CONTROL);
        const DWORD status = file ? SetSecurityInfo(file.get(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
            nullptr, nullptr, dacl, nullptr) : GetLastError();
        if (status) return Check(false, "prepare deny-write-attributes DACL", status);
    }
    fixture.attributes = GetFileAttributesW(fixture.Source().c_str());
    const auto refused = Open(fixture.Source(), FILE_WRITE_ATTRIBUTES);
    if (!Check(!refused && GetLastError() == ERROR_ACCESS_DENIED,
        "fixture denies initial WRITE_ATTRIBUTES while allowing approved read and retirement rights")) return false;
    HANDLE raw = nullptr;
    DWORD error = UacRenameLeaf::Detail::OpenSource(fixture.from.get(), fixture.leaf, raw,
        DELETE | READ_CONTROL | WRITE_DAC | FILE_READ_DATA | FILE_READ_EA, 0);
    Handle source(raw, CloseHandle);
    if (!error) error = UacMoveSourceJournal::PrepareRecovery(fixture.from.get(), fixture.leaf, source.get(), fixture.id,
        fixture.storage.get(), fixture.journal.get(), fixture.security.get(), fixture.alias, fixture.seal);
    source.reset();
    {
        UacMoveSourceJournal::SourceGuard guard(fixture.attributes, AllowFixture);
        if (!error) error = guard.Open(fixture.from.get(), fixture.leaf, fixture.id, 1);
        if (!error) error = guard.AddRecoveryLink(fixture.storage.get(), fixture.alias);
        if (!error) error = guard.LockStreams(fixture.storage.get(), fixture.alias);
        Check(!error, "prepare and guard a read-only source without demanding initial WRITE_ATTRIBUTES", error);
    }
    if (error) return false;
    error = fixture.Recover();
    return Check(!error && fixture.SourceIntact(), "restore the original deny-attributes DACL and read-only state after guarded preparation", error);
}

int ReadOnlyMoveCrash(const std::wstring& root, const std::wstring& name, bool verify)
{
    ReadOnlyMoveFixture fixture(root, name);
    if (!fixture.Init(!verify)) return 2;
    if (verify)
    {
        const DWORD error = fixture.Recover();
        const bool before = name == L"crash-before";
        const bool finished = name == L"crash-finished";
        Check(!error && (finished ? !Present(fixture.Source()) : fixture.SourceIntact()) &&
              (before ? !Present(fixture.Target()) : fixture.TargetMatches()) && NoStage(fixture.target),
              before ? "recover read-only source attributes and DACL after hard exit during data copy" : finished ?
                       "recognize completed read-only retirement after hard exit without resurrecting the source" :
                       "restore the exact read-only source from its recovery alias after hard exit during retirement", error);
        return failures ? 2 : 0;
    }
    DWORD error = fixture.Arm();
    UacMoveSourceJournal::SourceGuard guard(fixture.attributes, AllowFixture);
    if (!error) error = guard.Open(fixture.from.get(), fixture.leaf, fixture.id, 2);
    if (!error) error = guard.LockStreams(fixture.storage.get(), fixture.alias);
    Handle target(nullptr, CloseHandle); FILE_ID_INFO targetId{};
    const UacCopyLeaf::Detail::CopyProgress crash{
        [](void*, ULONGLONG, ULONGLONG) -> DWORD { TerminateProcess(GetCurrentProcess(), 87); return ERROR_GEN_FAILURE; }, nullptr};
    if (!error) error = guard.PublishCopy(fixture.to.get(), fixture.toId, fixture.leaf, fixture.size,
        nullptr, nullptr, MAXULONGLONG, target, targetId, name == L"crash-before" ? &crash : nullptr);
    UacMoveSourceJournal::crashAfterRemainingLinks = name == L"crash-finished" ? 0 : 1;
    if (!error) error = guard.RetirePublishedSource(fixture.from.get(), fixture.storage.get(), fixture.journal.get(), fixture.security.get(),
        fixture.to.get(), fixture.leaf, target.get(), targetId, nullptr, nullptr, MAXULONGLONG);
    Check(false, "reach read-only move hard-exit checkpoint", error);
    return 2;
}
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 3) return 2;
    const std::wstring root = std::wstring(argv[1]).rfind(L"\\\\?\\", 0) == 0 ? argv[1] : std::wstring(L"\\\\?\\") + argv[1];
    CopyFixture fixture(root);
    const std::wstring mode(argv[2]);
    if (mode.rfind(L"--move-crash-", 0) == 0)
        return ReadOnlyMoveCrash(root, L"crash-" + mode.substr(13), false);
    if (mode.rfind(L"--verify-move-crash-", 0) == 0)
        return ReadOnlyMoveCrash(root, L"crash-" + mode.substr(20), true);
    if (mode == L"--crash")
    {
        if (!fixture.OpenParents()) return 2;
        const UacCopyLeaf::Detail::PublicationGate crash{
            [](void*, HANDLE, ULONGLONG) -> DWORD { TerminateProcess(GetCurrentProcess(), 86); return ERROR_GEN_FAILURE; }, nullptr};
        const DWORD error = fixture.Copy(L"crash.bin", nullptr, &crash);
        std::printf("FAIL: did not reach process-exit checkpoint (%lu)\n", error);
        return 2;
    }
    if (mode == L"--verify-crash")
    {
        Check(!Present(fixture.target + L"\\crash.bin") && NoStage(fixture.target) &&
              AttributesMatch(fixture.source + L"\\large-東.bin", ordinaryAttributes) &&
              PatternMatches(fixture.source + L"\\large-東.bin", largeSize, 0x53) &&
              PatternMatches(fixture.source + L"\\large-東.bin:stream-日", namedSize, 0xa7),
              "hard process exit cleans the read-only stage and leaves source streams intact");
        return failures ? 2 : 0;
    }
    if (mode != L"--run") return 2;
    if (!SelectionCases() || !CopyCases(fixture) || !SparseCase(fixture) || !CompressedCase(fixture) ||
        !SameVolumeMoveCase(fixture) || !ReadOnlySecurityCase(fixture) || !MoveCase(fixture) ||
        !ReadOnlySameVolume(root) || !GuardedAttributeMove(root, L"guarded") || !GuardedAttributeMove(root, L"compressed", true) ||
        !GuardedAttributeMove(root, L"normal-no-archive", false, FILE_ATTRIBUTE_NORMAL) ||
        !GuardedAttributeMove(root, L"readonly-sparse", false, ordinaryAttributes | FILE_ATTRIBUTE_SPARSE_FILE) ||
        !ReadOnlyReceiptRetirement(root) || !MinimalMovePreparation(root))
    {
        if (!failures) Check(false, "prepare native fixture", GetLastError());
        return 2;
    }
    return failures ? 2 : 0;
}
