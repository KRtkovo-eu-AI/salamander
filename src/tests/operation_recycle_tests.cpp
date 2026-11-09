// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Bind recycle receipts to a volume GUID and the original account. | Reject wrong-account and wrong-volume evidence even through mounted folders. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Add an original-account Shell elevation probe. | Require an exact receipt and verify the restored file identity. | | GPT-6.1 Sol (Codex) |
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

static bool SameFileId(const FILE_ID_INFO& first, const FILE_ID_INFO& second)
{
    return first.VolumeSerialNumber == second.VolumeSerialNumber &&
           std::memcmp(first.FileId.Identifier, second.FileId.Identifier, sizeof(first.FileId.Identifier)) == 0;
}

static bool ReadFileId(const wchar_t* path, FILE_ID_INFO& id, std::wstring* normalizedPath = nullptr)
{
    HANDLE file = CreateFileW(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    bool result = GetFileInformationByHandleEx(file, FileIdInfo, &id, sizeof(id)) &&
                  GetFileInformationByHandleEx(file, FileAttributeTagInfo, &attributes, sizeof(attributes)) &&
                  !(attributes.FileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
    if (result && normalizedPath)
    {
        constexpr DWORD flags = FILE_NAME_NORMALIZED | VOLUME_NAME_GUID;
        const DWORD capacity = GetFinalPathNameByHandleW(file, nullptr, 0, flags);
        if (!capacity || capacity > 32768)
            result = false;
        else
        {
            normalizedPath->resize(capacity);
            const DWORD length = GetFinalPathNameByHandleW(file, normalizedPath->data(), capacity, flags);
            result = length && length < capacity;
            normalizedPath->resize(result ? length : 0);
        }
    }
    CloseHandle(file);
    return result;
}

static std::wstring_view VolumeRoot(std::wstring_view path)
{
    constexpr std::wstring_view prefix = LR"(\\?\Volume{)";
    constexpr size_t end = prefix.size() + 36;
    if (path.size() <= end + 1 || path.substr(0, prefix.size()) != prefix ||
        path[end] != L'}' || path[end + 1] != L'\\')
        return {};
    return path.substr(0, end + 2);
}

static bool InOriginalRecycleBin(std::wstring_view normalizedSource, std::wstring_view normalizedReceipt, const std::wstring& sid)
{
    const auto volume = VolumeRoot(normalizedSource);
    const auto parentEnd = normalizedReceipt.find_last_of(L'\\');
    if (volume.empty() || sid.empty() || parentEnd == std::wstring_view::npos)
        return false;
    const auto expected = std::wstring(volume) + LR"($Recycle.Bin\)" + sid;
    return CompareStringOrdinal(normalizedReceipt.data(), static_cast<int>(parentEnd),
                                expected.c_str(), static_cast<int>(expected.size()), TRUE) == CSTR_EQUAL;
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
        std::wstring normalizedSource;
        if (!ReadFileId(path.c_str(), before, &normalizedSource) || VolumeRoot(normalizedSource).empty())
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
        std::wstring normalizedReceipt;
        const bool sameFile = hasPath && ReadFileId(receiptPath, after, &normalizedReceipt) && SameFileId(before, after);
        const bool inCurrentBin = sameFile && InOriginalRecycleBin(normalizedSource, normalizedReceipt, sid);
        CoTaskMemFree(receiptPath);
        Check(SUCCEEDED(hr) && !present && inCurrentBin && sameFile,
              mode == 0 ? "Shell recycle preserves file identity in the caller's bin"
                        : "an alternate destination item does not change the NTFS Shell's bin");
    }
    return true;
}

static HRESULT FinishShellOperation(IFileOperation* operation)
{
    HRESULT hr = operation->PerformOperations();
    BOOL aborted = FALSE;
    if (SUCCEEDED(hr))
    {
        hr = operation->GetAnyOperationsAborted(&aborted);
        if (SUCCEEDED(hr) && aborted)
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    return hr;
}

static bool MatchesProbeContents(const std::filesystem::path& path, std::string_view expected)
{
    const HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                                    nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    LARGE_INTEGER size{};
    std::string actual(expected.size(), '\0');
    DWORD read = 0;
    const bool matches = GetFileSizeEx(file, &size) && size.QuadPart == static_cast<LONGLONG>(expected.size()) &&
        (actual.empty() || (ReadFile(file, actual.data(), static_cast<DWORD>(actual.size()), &read, nullptr) &&
                           read == actual.size())) && actual == expected;
    CloseHandle(file);
    return matches;
}

static bool ProbeRecycleReceipt(const std::filesystem::path& source, bool requestElevation, bool cancel, std::string_view expected)
{
    const auto sid = CurrentSid();
    FILE_ID_INFO initial{};
    std::wstring normalizedSource;
    if (sid.empty() || !ReadFileId(source.c_str(), initial, &normalizedSource) ||
        VolumeRoot(normalizedSource).empty() || !MatchesProbeContents(source, expected))
        return false;

    OperationRecycleProgress progress(true);
    IFileOperation* rawOperation = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&rawOperation));
    const ShellPtr<IFileOperation> operation(rawOperation);
    const DWORD elevation = requestElevation ? FOFX_SHOWELEVATIONPROMPT : 0;
    if (SUCCEEDED(hr))
        hr = operation->SetOperationFlags(OperationRecycleFlags | FOF_SILENT | elevation);
    IShellItem* rawSource = nullptr;
    if (SUCCEEDED(hr))
        hr = SHCreateItemFromParsingName(source.c_str(), nullptr, IID_PPV_ARGS(&rawSource));
    const ShellPtr<IShellItem> item(rawSource);
    if (SUCCEEDED(hr))
        hr = operation->DeleteItem(item.get(), &progress);
    if (SUCCEEDED(hr))
        hr = FinishShellOperation(operation.get());
    hr = progress.Result(hr, 1);
    std::printf("IFileOperation recycle=0x%08lX receipt=%d\n", static_cast<unsigned long>(hr), progress.Receipt() != nullptr);

    FILE_ID_INFO after{};
    if (cancel)
    {
        Check(hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) && !progress.Receipt() &&
                  ReadFileId(source.c_str(), after) && SameFileId(initial, after) && MatchesProbeContents(source, expected),
              "cancelled Windows elevation leaves the same source and no recycle receipt");
        return failures == 0;
    }
    if (FAILED(hr) || !progress.Receipt())
        return false;

    PWSTR rawPath = nullptr;
    const HRESULT pathResult = progress.Receipt()->GetDisplayName(SIGDN_FILESYSPATH, &rawPath);
    const std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)> receiptPath(rawPath, CoTaskMemFree);
    std::wstring normalizedReceipt;
    const bool same = SUCCEEDED(pathResult) && receiptPath && ReadFileId(receiptPath.get(), after, &normalizedReceipt) &&
                      SameFileId(initial, after);
    const bool inCallerBin = same && InOriginalRecycleBin(normalizedSource, normalizedReceipt, sid);
    const bool sourceAbsent = GetFileAttributesW(source.c_str()) == INVALID_FILE_ATTRIBUTES &&
                              GetLastError() == ERROR_FILE_NOT_FOUND;
    Check(sourceAbsent, "the original source name is absent after recycling");
    Check(inCallerBin, "the Shell receipt names the Recycle Bin of the original account");
    Check(same, "the Recycle Bin item has the original file ID");
    if (!requestElevation && inCallerBin)
    {
        auto anotherVolume = normalizedSource;
        constexpr size_t firstGuidDigit = std::wstring_view(LR"(\\?\Volume{)").size();
        anotherVolume[firstGuidDigit] = anotherVolume[firstGuidDigit] == L'0' ? L'1' : L'0';
        Check(!InOriginalRecycleBin(anotherVolume, normalizedReceipt, sid) &&
                  !InOriginalRecycleBin(normalizedSource, normalizedReceipt, sid + L"-1"),
              "reject a receipt from a different volume or account");
        const auto bin = std::wstring(VolumeRoot(normalizedSource)) + LR"(other\$Recycle.Bin\)" + sid;
        Check(!InOriginalRecycleBin(normalizedSource, bin + L"\\item", sid),
              "reject a lookalike bin nested under an unrelated folder");
    }
    if (!sourceAbsent || !inCallerBin || !same)
        return false;

    IShellItem* rawParent = nullptr;
    hr = SHCreateItemFromParsingName(source.parent_path().c_str(), nullptr, IID_PPV_ARGS(&rawParent));
    const ShellPtr<IShellItem> parent(rawParent);
    rawOperation = nullptr;
    if (SUCCEEDED(hr))
        hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&rawOperation));
    const ShellPtr<IFileOperation> restore(rawOperation);
    if (SUCCEEDED(hr))
        hr = restore->SetOperationFlags(FOF_NOERRORUI | FOFX_EARLYFAILURE | FOF_RENAMEONCOLLISION | elevation);
    if (SUCCEEDED(hr))
        hr = restore->MoveItem(progress.Receipt(), parent.get(), source.filename().c_str(), nullptr);
    if (SUCCEEDED(hr))
        hr = FinishShellOperation(restore.get());
    std::printf("IFileOperation restore=0x%08lX\n", static_cast<unsigned long>(hr));
    Check(SUCCEEDED(hr) && ReadFileId(source.c_str(), after) && SameFileId(initial, after) && MatchesProbeContents(source, expected),
          "restore the same item from the original user's Recycle Bin to its original name");
    return failures == 0;
}

static bool ProbeProtectedRecycle(bool cancel)
{
    PWSTR rawProgramFiles = nullptr;
    const HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT_PATH, nullptr, &rawProgramFiles);
    const std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)> programFiles(rawProgramFiles, CoTaskMemFree);
    if (FAILED(hr) || !programFiles)
        return false;
    const auto pilot = std::filesystem::path(programFiles.get()) / L"Open Samandarin UAC";
    const auto fixture = pilot / L"UacRecycleProbe";
    for (const auto& directory : {pilot, fixture})
    {
        const DWORD attributes = GetFileAttributesW(directory.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY) ||
            (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            return false;
    }
    const auto source = fixture / (cancel ? L"recycle-cancel.txt" : L"recycle-approve.txt");
    const DWORD attributes = GetFileAttributesW(source.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
        return false;
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;
    TOKEN_ELEVATION elevation{};
    DWORD bytes = 0;
    const BOOL tokenRead = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &bytes);
    CloseHandle(token);
    if (!tokenRead || elevation.TokenIsElevated)
    {
        std::puts("The protected recycle probe must run under the original unelevated account.");
        return false;
    }
    const HANDLE ordinary = CreateFileW(source.c_str(), DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (ordinary != INVALID_HANDLE_VALUE)
    {
        CloseHandle(ordinary);
        std::puts("The protected fixture can already be deleted without elevation.");
        return false;
    }
    if (GetLastError() != ERROR_ACCESS_DENIED)
        return false;
    return ProbeRecycleReceipt(source, true, cancel, "Samandarin UAC Recycle probe\n");
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
    if (argc == 2 && std::wstring_view(argv[1]) == L"--recycle-receipt-probe")
    {
        const auto source = root / L"ordinary-recycle-\u6771.txt";
        const bool complete = CreateDirectoryW(root.c_str(), nullptr) && MakeFile(source.wstring()) &&
                              ProbeRecycleReceipt(source, false, false, {});
        if (complete)
            DeleteFileW(source.c_str());
        RemoveDirectoryW(root.c_str());
        CoUninitialize();
        return complete && failures == 0 ? 0 : 1;
    }
    if (argc == 3 && std::wstring_view(argv[1]) == L"--elevation-probe")
    {
        const std::wstring_view mode(argv[2]);
        const bool complete = (mode == L"approve" || mode == L"cancel") && ProbeProtectedRecycle(mode == L"cancel");
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
    Check(FAILED(OperationRecyclePath(paths[0] + L":named", shellPath)) &&
              FAILED(OperationRecyclePath(LR"(C:relative.txt)", shellPath)) &&
              FAILED(OperationRecyclePath(LR"(\\.\C:\short.txt)", shellPath)) &&
              FAILED(OperationRecyclePath(LR"(\\?\GLOBALROOT\Device\HarddiskVolume1\short.txt)", shellPath)),
          "reject alternate stream, drive-relative and device aliases before queueing a Shell recycle");
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
