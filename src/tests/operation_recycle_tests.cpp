// SPDX-License-Identifier: GPL-2.0-or-later
// | 2026-11-08 | Probe the native Shell recycle destination and source guard. | Establish whether explicit destination or exclusive handles can enforce the required contract. | | GPT-6.1 Sol (Codex) |
// S20c adapted from PR #769: recycle-only fixture; shell/clipboard covered later.
#include "../operationrecycle.h"
#include <cstdio>
#include <cstring>
#include <string_view>
#include <filesystem>
#include <memory>
#include <sddl.h>
#include <shlguid.h>
#include <vector>

#pragma comment(lib, "advapi32.lib")
static int failures = 0;
static void Check(bool condition, const char* name)
{
    std::printf("%s %s\n", condition ? "PASS" : "FAIL", name);
    if (!condition) ++failures;
}
static bool MakeFile(const std::wstring& path)
{
    const std::wstring apiPath = path.size() >= MAX_PATH ? L"\\\\?\\" + path : path;
    HANDLE file = CreateFileW(apiPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    CloseHandle(file);
    return true;
}
template<class Interface>
struct ReleaseShell
{
    void operator()(Interface* value) const { if (value) value->Release(); }
};
template<class Interface>
using ShellPtr = std::unique_ptr<Interface, ReleaseShell<Interface>>;

static std::wstring CurrentSid()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return {};
    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    std::vector<BYTE> data(size);
    PWSTR sid = nullptr;
    if (size && GetTokenInformation(token, TokenUser, data.data(), size, &size))
        ConvertSidToStringSidW(reinterpret_cast<const TOKEN_USER*>(data.data())->User.Sid, &sid);
    CloseHandle(token);
    const std::wstring result = sid ? sid : L"";
    if (sid)
        LocalFree(sid);
    return result;
}

static bool ReadFileId(const wchar_t* path, FILE_ID_INFO& id)
{
    HANDLE file = CreateFileW(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    const BOOL result = GetFileInformationByHandleEx(file, FileIdInfo, &id, sizeof(id));
    CloseHandle(file);
    return result != FALSE;
}

static bool ProbeTransferRecycle(const std::filesystem::path& folder)
{
    if (!CreateDirectoryW(folder.c_str(), nullptr))
        return false;
    const auto explicitDestination = folder / L"other-destination";
    if (!CreateDirectoryW(explicitDestination.c_str(), nullptr))
        return false;

    IShellItem* raw = nullptr;
    HRESULT hr = SHCreateItemFromParsingName(folder.c_str(), nullptr, IID_PPV_ARGS(&raw));
    const ShellPtr<IShellItem> parent(raw);
    ITransferSource* rawTransfer = nullptr;
    if (SUCCEEDED(hr))
        hr = parent->BindToHandler(nullptr, BHID_Transfer, IID_PPV_ARGS(&rawTransfer));
    const ShellPtr<ITransferSource> transfer(rawTransfer);
    if (FAILED(hr))
    {
        std::printf("ITransferSource bind=0x%08lX\n", static_cast<unsigned long>(hr));
        return false;
    }
    const std::wstring sid = CurrentSid();
    if (sid.empty())
        return false;

    for (int mode = 0; mode != 3; ++mode)
    {
        const auto path = folder / (L"samandarin-transfer-" + std::to_wstring(mode) + L".txt");
        if (!MakeFile(path.wstring()))
            return false;
        FILE_ID_INFO before{};
        if (!ReadFileId(path.c_str(), before))
            return false;
        raw = nullptr;
        hr = SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&raw));
        const ShellPtr<IShellItem> source(raw);
        raw = nullptr;
        if (SUCCEEDED(hr))
        {
            hr = mode == 1
                ? SHCreateItemFromParsingName(explicitDestination.c_str(), nullptr, IID_PPV_ARGS(&raw))
                : SHGetKnownFolderItem(FOLDERID_RecycleBinFolder, KF_FLAG_DEFAULT, nullptr, IID_PPV_ARGS(&raw));
        }
        const ShellPtr<IShellItem> destination(raw);
        if (FAILED(hr))
            return false;

        HANDLE guard = INVALID_HANDLE_VALUE;
        if (mode == 2)
        {
            guard = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES | DELETE, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            if (guard == INVALID_HANDLE_VALUE)
                return false;
        }
        raw = nullptr;
        hr = transfer->RecycleItem(source.get(), destination.get(), TSF_DELETE_RECYCLE_IF_POSSIBLE, &raw);
        const ShellPtr<IShellItem> receipt(raw);
        if (guard != INVALID_HANDLE_VALUE)
            CloseHandle(guard);
        std::printf("ITransferSource case=%d result=0x%08lX receipt=%d\n", mode, static_cast<unsigned long>(hr),
                    receipt ? 1 : 0);
        const bool present = GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
        if (mode == 2)
        {
            Check(hr == COPYENGINE_E_SHARING_VIOLATION_SRC && !receipt && present,
                  "Shell cannot recycle the original while an exclusive delete handle pins its name");
            continue;
        }

        PWSTR receiptPath = nullptr;
        FILE_ID_INFO after{};
        const bool hasPath = receipt && SUCCEEDED(receipt->GetDisplayName(SIGDN_FILESYSPATH, &receiptPath)) && receiptPath;
        const auto recycleSid = L"\\$Recycle.Bin\\" + sid + L"\\";
        const bool inCurrentBin = hasPath && std::wstring_view(receiptPath).find(recycleSid) != std::wstring_view::npos;
        const bool sameFile = hasPath && ReadFileId(receiptPath, after) &&
                              before.VolumeSerialNumber == after.VolumeSerialNumber &&
                              std::memcmp(before.FileId.Identifier, after.FileId.Identifier, sizeof(before.FileId.Identifier)) == 0;
        CoTaskMemFree(receiptPath);
        Check(SUCCEEDED(hr) && !present && inCurrentBin && sameFile,
              mode == 0 ? "Shell recycle preserves file identity in the caller's bin"
                        : "an alternate destination item does not change the NTFS Shell's bin");
    }
    return true;
}

int wmain(int argc, wchar_t** argv)
{
    HRESULT initialized = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized)) return 2;
    std::vector<wchar_t> temp(32768);
    DWORD size = GetTempPathW((DWORD)temp.size(), temp.data());
    if (size == 0 || size >= temp.size()) return 2;
    std::filesystem::path testParent = std::filesystem::path(temp.data()).lexically_normal();
    if (testParent.filename().empty()) testParent = testParent.parent_path();
    const std::filesystem::path root = testParent /
        (L"samandarin-branch-operations-" + std::to_wstring(GetCurrentProcessId()) +
         L"-" + std::to_wstring(GetTickCount64()));
    std::error_code error;
    if (argc == 2 && std::wstring_view(argv[1]) == L"--transfer-probe")
    {
        const bool complete = ProbeTransferRecycle(root);
        if (root.parent_path() == testParent &&
            root.filename().wstring().find(L"samandarin-branch-operations-") == 0)
            std::filesystem::remove_all(std::filesystem::path(L"\\\\?\\" + root.wstring()), error);
        CoUninitialize();
        return complete && failures == 0 ? 0 : 1;
    }
    if (argc != 1)
    {
        CoUninitialize();
        return 2;
    }
    std::filesystem::create_directories(root / L"prvni-\u0161\u017e\u6f22", error);
    std::filesystem::create_directories(root / L"druhy-\u00e9\u03b1\U0001f600", error);
    std::filesystem::path longParent = root;
    for (int i = 0; i < 5; ++i) longParent /= std::wstring(60, L'\u6f22');
    std::filesystem::create_directories(std::filesystem::path(L"\\\\?\\" + longParent.wstring()), error);
    const std::vector<std::wstring> paths = {
        (root / L"prvni-\u0161\u017e\u6f22" / L"same-\u6f22.txt").wstring(),
        (root / L"druhy-\u00e9\u03b1\U0001f600" / L"same-\u6f22.txt").wstring(),
        (longParent / L"same-\u6f22.txt").wstring()};
    Check(MakeFile(paths[0]) && MakeFile(paths[1]) && MakeFile(paths[2]), "create duplicate Unicode basenames");
    Check(OperationRecycleDriveTypeW(LR"(\\?\)" + longParent.wstring()) == GetDriveTypeW(root.root_path().c_str()),
          "exact long Unicode volume retains ordinary root drive type");
    std::wstring shellPath;
    Check(SUCCEEDED(OperationRecyclePath(LR"(\\?\C:\short.txt)", shellPath)) && shellPath == LR"(C:\short.txt)",
          "recycle strips extended prefix even on short ASCII paths");
    Check(SUCCEEDED(OperationRecyclePath(LR"(\\?\UNC\server\share\file.txt)", shellPath)) && shellPath == LR"(\\server\share\file.txt)",
          "recycle preserves UNC root while stripping transport prefix");
    Check(FAILED(OperationRecyclePath(paths[0] + L".", shellPath)) &&
          FAILED(OperationRecyclePath(root.wstring() + LR"(\parent.\same.txt)", shellPath)),
          "recycle rejects both leaf and ancestor shell-normalization aliases");
    const std::wstring byteLongPath = (root / (std::wstring(100, L'\u6f22') + L".txt")).wstring();
    Check(byteLongPath.size() < MAX_PATH && MakeFile(byteLongPath), "create UTF8-over-260 fixture with short UTF16 path");
    const std::vector<std::wstring> recyclePaths = {
        (root / L"short.txt").wstring(), paths[0], paths[1], paths[2], byteLongPath};
    Check(MakeFile(recyclePaths[0]), "create short ordinary-panel recycle fixture");
    for (size_t i = 0; i < recyclePaths.size(); ++i)
    {
        const std::wstring exact = LR"(\\?\)" + recyclePaths[i];
        DWORD recycled = RecycleOperationFileW(NULL, exact);
        char label[100];
        sprintf_s(label, "actual selected-extension recycle preserves exact path %zu (%lu)", i, recycled);
        Check(recycled == ERROR_SUCCESS && GetFileAttributesW(exact.c_str()) == INVALID_FILE_ATTRIBUTES, label);
    }
    Check(RecycleOperationFileW(NULL, LR"(\\?\)" + root.wstring() + LR"(\missing.txt)") != ERROR_SUCCESS,
          "missing recycle item cannot silently succeed");
    const std::wstring lockedPath = (root / L"locked.txt").wstring();
    HANDLE locked = CreateFileW(lockedPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    Check(locked != INVALID_HANDLE_VALUE, "create locked recycle fixture");
    Check(RecycleOperationFileW(NULL, lockedPath) != ERROR_SUCCESS &&
          GetFileAttributesW(lockedPath.c_str()) != INVALID_FILE_ATTRIBUTES,
          "failed per-item recycle cannot silently report success");
    if (locked != INVALID_HANDLE_VALUE) CloseHandle(locked);
    // Verify the resolved target remains our newly created direct child.
    if (root.parent_path() == testParent &&
        root.filename().wstring().find(L"samandarin-branch-operations-") == 0)
        std::filesystem::remove_all(std::filesystem::path(L"\\\\?\\" + root.wstring()), error);
    CoUninitialize();
    return failures == 0 ? 0 : 1;
}
