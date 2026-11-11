// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Isolate native file-list workspace implementation. | Keep newer Windows identity types and NT headers out of the host PCH. | #731 | GPT-6.1 Sol |
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "uacfilelist.h"
#include <objbase.h>
#include <sddl.h>
#include <bcrypt.h>
#include <algorithm>
#include <cstring>
#include <vector>
#include "uacbroker/directoryleaf.h"
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "ntdll.lib")

namespace UacFileList
{
#ifdef SAM_UAC_FILELIST_TEST
DWORD CleanupFaultForTest(unsigned point);
DWORD IdentityFaultForTest(unsigned point);
#endif
class Workspace::Impl
{
public:
    using CleanupReporter = void (*)(void*, const std::wstring&, DWORD);
    using CleanupFallback = void (*)(void*, const std::wstring&, DWORD) noexcept;
    explicit Impl(CleanupReporter reporter = nullptr, void* context = nullptr,
                       CleanupFallback fallback = nullptr)
        : reporter_(reporter), reporterContext_(context), fallback_(fallback) {}
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    ~Impl()
    {
        const DWORD error = Cleanup();
        if (error && reporter_)
        {
            try { reporter_(reporterContext_, directoryPath_, error); }
            catch (...) { if (fallback_) fallback_(reporterContext_, directoryPath_, error); }
        }
    }

    DWORD Create()
    {
        std::vector<wchar_t> temporary(32768);
        const DWORD length = GetTempPathW(static_cast<DWORD>(temporary.size()), temporary.data());
        if (!length) return GetLastError();
        if (length >= temporary.size()) return ERROR_FILENAME_EXCED_RANGE;
        std::wstring rootPath(temporary.data());
        if (rootPath.size() >= MAX_PATH && rootPath.compare(0, 4, L"\\\\?\\") != 0)
            rootPath = rootPath.compare(0, 2, L"\\\\") == 0
                ? L"\\\\?\\UNC\\" + rootPath.substr(2) : L"\\\\?\\" + rootPath;
        root_ = CreateFileW(rootPath.c_str(), FILE_ADD_SUBDIRECTORY | FILE_READ_ATTRIBUTES | FILE_TRAVERSE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (root_ == INVALID_HANDLE_VALUE) return GetLastError();
        UacDirectoryLeaf::ParentIdentity rootIdentity;
        DWORD error = UacDirectoryLeaf::ReadParentIdentity(root_, rootIdentity);
        if (error) return error;

        HANDLE rawToken = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &rawToken)) return GetLastError();
        const std::unique_ptr<void, decltype(&CloseHandle)> token(rawToken, CloseHandle);
        alignas(TOKEN_USER) BYTE buffer[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE]{};
        DWORD used = 0;
        if (!GetTokenInformation(token.get(), TokenUser, buffer, sizeof(buffer), &used)) return GetLastError();
        LPWSTR rawSid = nullptr;
        if (!ConvertSidToStringSidW(reinterpret_cast<const TOKEN_USER*>(buffer)->User.Sid, &rawSid)) return GetLastError();
        const std::unique_ptr<void, decltype(&LocalFree)> sid(rawSid, LocalFree);
        // The originating user owns the workspace. An elevated administrator
        // can read, but cannot change or delete, its contents through this ACL.
        const std::wstring sddl = std::wstring(L"D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;") + rawSid +
                                  L")(A;OICI;0x001200A9;;;BA)";
        PSECURITY_DESCRIPTOR rawSecurity = nullptr;
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1,
                                                                   &rawSecurity, nullptr)) return GetLastError();
        const std::unique_ptr<void, decltype(&LocalFree)> security(rawSecurity, LocalFree);
        GUID guid{};
        wchar_t name[39]{};
        if (FAILED(CoCreateGuid(&guid)) || !StringFromGUID2(guid, name, ARRAYSIZE(name))) return ERROR_GEN_FAILURE;
        directoryName_ = std::wstring(L"Samandarin-FileList-") + name;
        directoryPath_ = std::wstring(temporary.data()) + directoryName_;
        filePath_ = directoryPath_ + L"\\list.tmp";
        error = UacDirectoryLeaf::Detail::NativeDirectory(root_, directoryName_, FILE_CREATE, directory_,
            FILE_ADD_FILE | DELETE, rawSecurity, FILE_SHARE_READ | FILE_SHARE_WRITE);
        if (error) { directory_ = INVALID_HANDLE_VALUE; return error; }
        if (const DWORD identityError = ReadIdentity(directory_, parentIdentity_, 1)) return identityError;
        UNICODE_STRING child{};
        constexpr wchar_t childName[] = L"list.tmp";
        child.Length = static_cast<USHORT>((ARRAYSIZE(childName) - 1) * sizeof(wchar_t));
        child.MaximumLength = child.Length;
        child.Buffer = const_cast<PWSTR>(childName);
        OBJECT_ATTRIBUTES oa{};
        InitializeObjectAttributes(&oa, &child, OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE, directory_, nullptr);
        IO_STATUS_BLOCK status{};
        const NTSTATUS created = NtCreateFile(&writer_, GENERIC_READ | GENERIC_WRITE | DELETE | SYNCHRONIZE,
            &oa, &status, nullptr, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_CREATE,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT, nullptr, 0);
        if (created < 0) { writer_ = INVALID_HANDLE_VALUE; return RtlNtStatusToDosError(created); }
        if (const DWORD identityError = ReadIdentity(writer_, identity_, 2)) return identityError;
        return ERROR_SUCCESS;
    }

    HANDLE Writer() const { return writer_; }
    const std::wstring& Path() const { return filePath_; }
    const std::wstring& DirectoryName() const { return directoryName_; }
    ULONGLONG Size() const { return size_; }
    const FILE_ID_INFO& Identity() const { return identity_; }
    const FILE_ID_INFO& ParentIdentity() const { return parentIdentity_; }
#ifdef SAM_UAC_FILELIST_TEST
    void ReleaseReaderForTest()
    {
        if (reader_ != INVALID_HANDLE_VALUE) CloseHandle(reader_);
        reader_ = INVALID_HANDLE_VALUE;
    }
#endif

    DWORD Seal()
    {
        if (writer_ == INVALID_HANDLE_VALUE || directory_ == INVALID_HANDLE_VALUE) return ERROR_INVALID_HANDLE;
        LARGE_INTEGER length{};
        if (!FlushFileBuffers(writer_) || !GetFileSizeEx(writer_, &length)) return GetLastError();
        if (length.QuadPart < 0) return ERROR_FILE_INVALID;
        size_ = static_cast<ULONGLONG>(length.QuadPart);
        BYTE before[32]{}, after[32]{};
        DWORD error = Hash(writer_, size_, before);
        if (error) return error;
        CloseHandle(writer_); writer_ = INVALID_HANDLE_VALUE;
        error = OpenChild(GENERIC_READ, FILE_SHARE_READ, reader_);
        if (error) return error;
        FILE_ID_INFO actual{};
        FILE_ATTRIBUTE_TAG_INFO attributes{};
        LARGE_INTEGER current{};
        if (!GetFileInformationByHandleEx(reader_, FileIdInfo, &actual, sizeof(actual)) ||
            !GetFileInformationByHandleEx(reader_, FileAttributeTagInfo, &attributes, sizeof(attributes)) ||
            !GetFileSizeEx(reader_, &current)) return GetLastError();
        if (actual.VolumeSerialNumber != identity_.VolumeSerialNumber ||
            memcmp(actual.FileId.Identifier, identity_.FileId.Identifier, sizeof(identity_.FileId.Identifier)) ||
            (attributes.FileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) ||
            current.QuadPart != length.QuadPart) return ERROR_FILE_INVALID;
        error = Hash(reader_, size_, after);
        return error ? error : memcmp(before, after, sizeof(before)) ? ERROR_FILE_INVALID : ERROR_SUCCESS;
    }

private:
    static DWORD ReadIdentity(HANDLE file, FILE_ID_INFO& identity, unsigned point)
    {
#ifdef SAM_UAC_FILELIST_TEST
        if (const DWORD fault = IdentityFaultForTest(point)) return fault;
#else
        (void)point;
#endif
        return GetFileInformationByHandleEx(file, FileIdInfo, &identity, sizeof(identity))
            ? ERROR_SUCCESS : GetLastError();
    }

    DWORD OpenChild(ACCESS_MASK access, ULONG sharing, HANDLE& file) const
    {
        constexpr wchar_t childName[] = L"list.tmp";
        UNICODE_STRING child{};
        child.Length = static_cast<USHORT>((ARRAYSIZE(childName) - 1) * sizeof(wchar_t));
        child.MaximumLength = child.Length;
        child.Buffer = const_cast<PWSTR>(childName);
        OBJECT_ATTRIBUTES oa{};
        InitializeObjectAttributes(&oa, &child, OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE, directory_, nullptr);
        IO_STATUS_BLOCK io{};
        file = nullptr;
        const NTSTATUS status = NtCreateFile(&file, access | SYNCHRONIZE, &oa, &io, nullptr,
            FILE_ATTRIBUTE_NORMAL, sharing, FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT, nullptr, 0);
        if (status < 0) { file = INVALID_HANDLE_VALUE; return RtlNtStatusToDosError(status); }
        return ERROR_SUCCESS;
    }

    static DWORD Hash(HANDLE file, ULONGLONG bytes, BYTE (&digest)[32])
    {
        BCRYPT_ALG_HANDLE provider = nullptr;
        BCRYPT_HASH_HANDLE hash = nullptr;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&provider, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
        if (status < 0) return RtlNtStatusToDosError(status);
        status = BCryptCreateHash(provider, &hash, nullptr, 0, nullptr, 0, 0);
        DWORD ioError = ERROR_SUCCESS;
        if (status >= 0)
        {
            LARGE_INTEGER start{};
            if (!SetFilePointerEx(file, start, nullptr, FILE_BEGIN)) ioError = GetLastError();
            BYTE block[4096];
            while (!ioError && status >= 0 && bytes)
            {
                const DWORD count = static_cast<DWORD>((std::min)(bytes, static_cast<ULONGLONG>(sizeof(block))));
                DWORD read = 0;
                if (!ReadFile(file, block, count, &read, nullptr)) { ioError = GetLastError(); break; }
                if (read != count) { ioError = ERROR_HANDLE_EOF; break; }
                status = BCryptHashData(hash, block, count, 0);
                bytes -= count;
            }
            if (!ioError && status >= 0) status = BCryptFinishHash(hash, digest, sizeof(digest), 0);
            BCryptDestroyHash(hash);
        }
        BCryptCloseAlgorithmProvider(provider, 0);
        return ioError ? ioError : status < 0 ? RtlNtStatusToDosError(status) : ERROR_SUCCESS;
    }

    DWORD Cleanup()
    {
        DWORD error = ERROR_SUCCESS;
        if (writer_ != INVALID_HANDLE_VALUE)
        {
            // This is the still-open handle returned by FILE_CREATE, even if
            // reading its identity failed. Never resolve a replacement name.
            FILE_DISPOSITION_INFO remove{TRUE};
            error = MarkForDeletion(writer_, remove, 1);
            CloseHandle(writer_); writer_ = INVALID_HANDLE_VALUE;
        }
        if (reader_ != INVALID_HANDLE_VALUE) { CloseHandle(reader_); reader_ = INVALID_HANDLE_VALUE; }
        if (directory_ != INVALID_HANDLE_VALUE)
        {
            HANDLE candidate = INVALID_HANDLE_VALUE;
            DWORD openError = ERROR_FILE_NOT_FOUND;
            // A broker whose outcome is unknown may still hold the source.
            // Do not wait without a bound, and never delete a replacement.
            for (unsigned attempt = 0; !filePath_.empty() && attempt <= 20; ++attempt)
            {
                openError = OpenChild(DELETE | FILE_READ_ATTRIBUTES,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, candidate);
                if (openError != ERROR_SHARING_VIOLATION || attempt == 20) break;
                Sleep(50);
            }
            if (!openError)
            {
                FILE_ID_INFO actual{};
                if (!GetFileInformationByHandleEx(candidate, FileIdInfo, &actual, sizeof(actual)))
                { if (!error) error = GetLastError(); }
                else if (actual.VolumeSerialNumber == identity_.VolumeSerialNumber &&
                         !memcmp(actual.FileId.Identifier, identity_.FileId.Identifier, sizeof(identity_.FileId.Identifier)))
                {
                    FILE_DISPOSITION_INFO remove{TRUE};
                    const DWORD removed = MarkForDeletion(candidate, remove, 2);
                    // A successfully reopened exact ID can complete cleanup
                    // even if the first deletion through the writer failed.
                    error = removed;
                }
                else if (!error) error = ERROR_FILE_INVALID;
                CloseHandle(candidate);
            }
            else if (openError == ERROR_FILE_NOT_FOUND) error = ERROR_SUCCESS;
            else if (!error) error = openError;
            FILE_DISPOSITION_INFO remove{TRUE};
            const DWORD removed = MarkForDeletion(directory_, remove, 3);
            if (removed && !error) error = removed;
        }
        if (directory_ != INVALID_HANDLE_VALUE) CloseHandle(directory_);
        if (root_ != INVALID_HANDLE_VALUE) CloseHandle(root_);
        directory_ = root_ = INVALID_HANDLE_VALUE;
        return error;
    }

    static DWORD MarkForDeletion(HANDLE file, FILE_DISPOSITION_INFO& remove, unsigned point)
    {
#ifdef SAM_UAC_FILELIST_TEST
        if (const DWORD fault = CleanupFaultForTest(point)) return fault;
#else
        (void)point;
#endif
        return SetFileInformationByHandle(file, FileDispositionInfo, &remove, sizeof(remove))
            ? ERROR_SUCCESS : GetLastError();
    }

    HANDLE root_ = INVALID_HANDLE_VALUE, directory_ = INVALID_HANDLE_VALUE;
    HANDLE writer_ = INVALID_HANDLE_VALUE, reader_ = INVALID_HANDLE_VALUE;
    std::wstring directoryName_, directoryPath_, filePath_;
    FILE_ID_INFO identity_{};
    FILE_ID_INFO parentIdentity_{};
    ULONGLONG size_ = 0;
    CleanupReporter reporter_ = nullptr;
    void* reporterContext_ = nullptr;
    CleanupFallback fallback_ = nullptr;
};

Workspace::Workspace(CleanupReporter reporter, void* context, CleanupFallback fallback)
    : impl_(std::make_unique<Impl>(reporter, context, fallback)) {}
Workspace::~Workspace() = default;
DWORD Workspace::Create() { return impl_->Create(); }
DWORD Workspace::Seal() { return impl_->Seal(); }
HANDLE Workspace::Writer() const { return impl_->Writer(); }
const std::wstring& Workspace::Path() const { return impl_->Path(); }
const std::wstring& Workspace::DirectoryName() const { return impl_->DirectoryName(); }
ULONGLONG Workspace::Size() const { return impl_->Size(); }
static UacF7::MoveFileIdentity PortableIdentity(const FILE_ID_INFO& identity)
{
    UacF7::MoveFileIdentity portable{};
    portable.serial = identity.VolumeSerialNumber;
    std::memcpy(portable.id, identity.FileId.Identifier, sizeof(portable.id));
    return portable;
}
UacF7::MoveFileIdentity Workspace::Identity() const { return PortableIdentity(impl_->Identity()); }
UacF7::MoveFileIdentity Workspace::ParentIdentity() const { return PortableIdentity(impl_->ParentIdentity()); }
#ifdef SAM_UAC_FILELIST_TEST
void Workspace::ReleaseReaderForTest() { impl_->ReleaseReaderForTest(); }
#endif
}
