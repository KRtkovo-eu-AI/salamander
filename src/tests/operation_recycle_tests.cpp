// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Test the original-account Shell elevation policy. | Reject standard tokens and credential prompts before retrying protected recycling. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Cover the two Shell callbacks around elevation. | Distinguish intermediate elevation from a final result and recheck the source. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Exercise late Shell results with a real recycle receipt. | Accept completion only when the original account and exact file match. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Check the shared recycle completion contract. | Exercise the original-account receipt with a real Windows recycle operation. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Exercise the verified Shell recycle source gate. | Refuse a substituted file without moving either object. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Test exact Shell recycle path admission. | Reject paths that would be reinterpreted after removing the Win32 prefix. | | GPT-6.1 Sol (Codex) |
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

using namespace OperationRecycleDetail;

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
        RecycleFileId before{};
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
        RecycleFileId after{};
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
    VerifiedSource original;
    if (sid.empty() || !ReadFileId(source.c_str(), original.id, &original.normalizedPath) ||
        VolumeRoot(original.normalizedPath).empty() || !MatchesProbeContents(source, expected))
        return false;

    constexpr HRESULT cancelled = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    constexpr HRESULT invalid = HRESULT_FROM_WIN32(ERROR_FILE_INVALID);
    if (!requestElevation)
    {
        IShellItem* item = nullptr;
        const auto itemResult = SHCreateItemFromParsingName(source.c_str(), nullptr, IID_PPV_ARGS(&item));
        constexpr DWORD recycle = TSF_DELETE_RECYCLE_IF_POSSIBLE;
        const auto elevation = COPYENGINE_E_REQUIRES_ELEVATION;
        OperationRecycleProgress ordinary(true, &original);
        Check(ordinary.PostDeleteItem(recycle, item, elevation, nullptr) == elevation &&
                  ordinary.Result(S_OK, 1) == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED),
              "ordinary recycling still rejects a result that requires elevation");
        OperationRecycleProgress retry(true, &original, true);
        Check(SUCCEEDED(itemResult) && retry.PreDeleteItem(recycle, item) == S_OK &&
                  retry.PostDeleteItem(recycle, item, elevation, nullptr) == S_OK &&
                  retry.PreDeleteItem(recycle, item) == S_OK && !retry.Receipt(),
              "accept the intermediate elevation result and revalidate the same source");
        Check(retry.Result(S_OK, 1) == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) &&
                  retry.Result(cancelled, 1) == cancelled,
              "require a confirmed receipt and preserve cancellation after an elevation request");
        Check(retry.PreDeleteItem(0, item) == HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED),
              "refuse a permanent delete even after an elevation request");
        OperationRecycleProgress replaced(true, &original, true);
        Check(replaced.PostDeleteItem(recycle, item, elevation, nullptr) == S_OK &&
                  replaced.PreDeleteItem(recycle, nullptr) == invalid,
              "reject a missing or changed Shell item on the elevated retry");
        if (item) item->Release();
    }
    Check(original.CompleteRecycle(source.wstring(), nullptr, sid, cancelled) == cancelled,
          "keep cancellation when the original file is still present and no receipt exists");
    OperationRecycleProgress progress(true, &original, requestElevation);
    std::wstring shellPath;
    HRESULT hr = OperationRecyclePath(source.wstring(), shellPath);
    if (SUCCEEDED(hr))
        hr = RunSingleRecycleOperation(nullptr, shellPath, progress, requestElevation);
    std::printf("IFileOperation recycle=0x%08lX receipt=%d\n", static_cast<unsigned long>(hr), progress.Receipt() != nullptr);

    RecycleFileId after{};
    if (cancel)
    {
        Check(hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) && !progress.Receipt() &&
                  ReadFileId(source.c_str(), after) && SameFileId(original.id, after) && MatchesProbeContents(source, expected),
              "cancelled Windows elevation leaves the same source and no recycle receipt");
        return failures == 0;
    }
    if (FAILED(hr) || !progress.Receipt())
        return false;

    std::wstring normalizedReceipt;
    const bool same = ReadShellItemIdentity(progress.Receipt(), after, normalizedReceipt) && SameFileId(original.id, after);
    const bool inCallerBin = same && InOriginalRecycleBin(original.normalizedPath, normalizedReceipt, sid);
    const bool sourceAbsent = GetFileAttributesW(source.c_str()) == INVALID_FILE_ATTRIBUTES &&
                              GetLastError() == ERROR_FILE_NOT_FOUND;
    Check(sourceAbsent, "the original source name is absent after recycling");
    Check(inCallerBin, "the Shell receipt names the Recycle Bin of the original account");
    Check(same, "the Recycle Bin item has the original file ID");
    if (!requestElevation && inCallerBin)
    {
        auto anotherVolume = original.normalizedPath;
        constexpr size_t firstGuidDigit = std::wstring_view(LR"(\\?\Volume{)").size();
        anotherVolume[firstGuidDigit] = anotherVolume[firstGuidDigit] == L'0' ? L'1' : L'0';
        Check(!InOriginalRecycleBin(anotherVolume, normalizedReceipt, sid) &&
                  !InOriginalRecycleBin(original.normalizedPath, normalizedReceipt, sid + L"-1"),
              "reject a receipt from a different volume or account");
        const auto bin = std::wstring(VolumeRoot(original.normalizedPath)) + LR"(other\$Recycle.Bin\)" + sid;
        Check(!InOriginalRecycleBin(original.normalizedPath, bin + L"\\item", sid),
              "reject a lookalike bin nested under an unrelated folder");
    }
    Check(original.ConfirmRecycled(progress.Receipt(), sid),
          "the complete receipt check confirms the original account, identity and removal of the old name");
    Check(!original.ConfirmRecycled(progress.Receipt(), sid + L"-1") &&
              !original.ConfirmRecycled(nullptr, sid),
          "the complete receipt check rejects a different account or a missing receipt");
    Check(original.CompleteRecycle(source.wstring(), progress.Receipt(), sid, E_FAIL) == S_OK &&
              original.CompleteRecycle(source.wstring(), progress.Receipt(), sid, cancelled) == S_OK,
          "keep confirmed recycle completion after a late aggregate error or cancellation");
    Check(original.CompleteRecycle(source.wstring(), nullptr, sid, cancelled) == invalid &&
              original.CompleteRecycle(source.wstring(), progress.Receipt(), sid + L"-1", S_OK) == invalid,
          "refuse a missing or wrong-account receipt when the source is gone");
    OperationRecycleProgress retried(true, &original, true);
    constexpr DWORD recycle = TSF_DELETE_RECYCLE_IF_POSSIBLE;
    Check(retried.PostDeleteItem(recycle, nullptr, COPYENGINE_E_REQUIRES_ELEVATION, nullptr) == S_OK &&
              retried.PostDeleteItem(recycle, nullptr, S_OK, progress.Receipt()) == S_OK &&
              retried.Result(S_OK, 1) == S_OK && original.ConfirmRecycled(retried.Receipt(), sid),
          "accept the verified receipt after an intermediate elevation request");
    OperationRecycleProgress duplicate(true), failed(true);
    duplicate.PostDeleteItem(0, nullptr, S_OK, progress.Receipt());
    duplicate.PostDeleteItem(0, nullptr, S_OK, progress.Receipt());
    failed.PostDeleteItem(0, nullptr, S_OK, progress.Receipt());
    failed.PostDeleteItem(0, nullptr, E_FAIL, nullptr);
    Check(!duplicate.Receipt() && !failed.Receipt(),
          "refuse the receipt if the single-item callback count or outcome is inconsistent");
    if (!sourceAbsent || !inCallerBin || !same)
        return false;

    IShellItem* rawParent = nullptr;
    hr = SHCreateItemFromParsingName(source.parent_path().c_str(), nullptr, IID_PPV_ARGS(&rawParent));
    const ShellPtr<IShellItem> parent(rawParent);
    IFileOperation* rawOperation = nullptr;
    const DWORD elevation = requestElevation ? FOFX_SHOWELEVATIONPROMPT : 0;
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
    Check(SUCCEEDED(hr) && ReadFileId(source.c_str(), after) && SameFileId(original.id, after) && MatchesProbeContents(source, expected),
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
    auto truncated = paths[0];
    truncated.append({L'\0', L'x'});
    Check(FAILED(OperationRecyclePath(truncated, shellPath)) &&
              FAILED(OperationRecyclePath(L"C:\\file\t.txt", shellPath)),
          "reject embedded NUL and control characters before Shell parsing");
    Check(FAILED(OperationRecyclePath(LR"(\\?\C:\folder/file.txt)", shellPath)) &&
              FAILED(OperationRecyclePath(LR"(C:\folder\\file.txt)", shellPath)) &&
              FAILED(OperationRecyclePath(LR"(C:\)", shellPath)) &&
              FAILED(OperationRecyclePath(LR"(\\?\\\server\share\file.txt)", shellPath)) &&
              FAILED(OperationRecyclePath(LR"(\\server\share)", shellPath)),
          "reject alternate separators, empty components and roots before Shell parsing");
    Check(FAILED(OperationRecyclePath(LR"(\\?\C:\folder\nUl .txt)", shellPath)) &&
              FAILED(OperationRecyclePath(L"C:\\folder\\CoM\u00B9.bin", shellPath)) &&
              FAILED(OperationRecyclePath(LR"(C:\Lpt9\file.txt)", shellPath)),
          "reject DOS device aliases before Shell parsing");
    Check(SUCCEEDED(OperationRecyclePath(LR"(C:\auxiliary\nul-backup.txt)", shellPath)) &&
              SUCCEEDED(OperationRecyclePath(LR"(\\server\share\directory\file.txt)", shellPath)),
          "keep ordinary drive and UNC file paths");
    Check(IsSameAccountRecyclePolicy(TokenElevationTypeLimited, 0) &&
              IsSameAccountRecyclePolicy(TokenElevationTypeLimited, 2) &&
              IsSameAccountRecyclePolicy(TokenElevationTypeLimited, 4) &&
              IsSameAccountRecyclePolicy(TokenElevationTypeLimited, 5),
          "admit only same-account elevation or consent policies for a split token");
    Check(!IsSameAccountRecyclePolicy(TokenElevationTypeLimited, 1) &&
              !IsSameAccountRecyclePolicy(TokenElevationTypeLimited, 3) &&
              !IsSameAccountRecyclePolicy(TokenElevationTypeLimited, 6) &&
              !IsSameAccountRecyclePolicy(TokenElevationTypeDefault, 5) &&
              !IsSameAccountRecyclePolicy(TokenElevationTypeFull, 5),
          "reject credential policies, unknown policy values and non-split tokens");
    const auto stale = root / L"stale-recycle.txt";
    const auto earlier = root / L"earlier-recycle.txt";
    VerifiedSource original;
    if (!MakeFile(stale.wstring()) || !ReadFileId(stale.c_str(), original.id, &original.normalizedPath) ||
        !MoveFileExW(stale.c_str(), earlier.c_str(), 0) || !MakeFile(stale.wstring()))
        return 2;
    OperationRecycleProgress checked(true, &original);
    RecycleFileId earlierId{};
    Check(RunSingleRecycleOperation(nullptr, stale.wstring(), checked) == HRESULT_FROM_WIN32(ERROR_FILE_INVALID) &&
              !checked.Receipt() && GetFileAttributesW(stale.c_str()) != INVALID_FILE_ATTRIBUTES &&
              ReadFileId(earlier.c_str(), earlierId) && SameFileId(original.id, earlierId),
          "reject a source replaced after capture without recycling either file");
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
