// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Share the single-item Shell recycle and receipt checks. | Validate exact source identity before the Shell starts processing. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Reject Shell path normalization aliases before recycling. | Keep exact disk paths separate from Shell and DOS namespaces. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Reject alternate stream and device aliases before recycling. | Pass the Shell only absolute filesystem paths. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Allow an exact Shell recycle receipt to be inspected. | Verify account and identity without changing ordinary recycling. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Guard Windows Shell recycling against permanent fallback. | Require recycle intent and a per-item Recycle Bin receipt. | | GPT-6.1 Sol (Codex) |
#pragma once
#include <windows.h>
#include <shobjidl.h>
#include <sherrors.h>
#include <shlobj.h>
#include <shellapi.h>
#include <sddl.h>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <vector>

// Keep recycling in the calling user's Shell context. Access failures stop the
// operation without requesting a different user's token.
inline constexpr DWORD OperationRecycleFlags = FOF_ALLOWUNDO | FOF_NOERRORUI | FOFX_RECYCLEONDELETE | FOFX_ADDUNDORECORD |
                                              FOFX_EARLYFAILURE | FOF_WANTNUKEWARNING | FOF_RENAMEONCOLLISION;

inline HRESULT NormalizeRecycleResult(HRESULT result)
{
    switch (result)
    {
    case COPYENGINE_E_USER_CANCELLED:
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    case COPYENGINE_E_REQUIRES_ELEVATION:
    case COPYENGINE_E_ACCESS_DENIED_SRC:
    case COPYENGINE_E_ACCESS_DENIED_DEST:
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    default:
        return result;
    }
}

namespace OperationRecycleDetail
{
inline std::wstring CurrentSid()
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

// The application's SDK target hides FileIdInfo. Use its documented layout
// for the opt-in NTFS identity check without raising the application target.
struct RecycleFileId
{
    ULONGLONG VolumeSerialNumber;
    BYTE FileId[16];
};
constexpr auto recycleFileIdInfo = static_cast<FILE_INFO_BY_HANDLE_CLASS>(18);
#if NTDDI_VERSION >= NTDDI_WIN8
static_assert(sizeof(RecycleFileId) == sizeof(FILE_ID_INFO) && recycleFileIdInfo == FileIdInfo);
#endif

inline bool SameFileId(const RecycleFileId& first, const RecycleFileId& second)
{
    return first.VolumeSerialNumber == second.VolumeSerialNumber &&
           std::memcmp(first.FileId, second.FileId, sizeof(first.FileId)) == 0;
}

inline bool ReadFileId(const wchar_t* path, RecycleFileId& id, std::wstring* normalizedPath = nullptr)
{
    HANDLE file = CreateFileW(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    bool result = GetFileInformationByHandleEx(file, recycleFileIdInfo, &id, sizeof(id)) &&
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

inline std::wstring_view VolumeRoot(std::wstring_view path)
{
    constexpr std::wstring_view prefix = LR"(\\?\Volume{)";
    constexpr size_t end = prefix.size() + 36;
    if (path.size() <= end + 1 || path.substr(0, prefix.size()) != prefix ||
        path[end] != L'}' || path[end + 1] != L'\\')
        return {};
    return path.substr(0, end + 2);
}

inline bool InOriginalRecycleBin(std::wstring_view normalizedSource, std::wstring_view normalizedReceipt, const std::wstring& sid)
{
    const auto volume = VolumeRoot(normalizedSource);
    const auto parentEnd = normalizedReceipt.find_last_of(L'\\');
    if (volume.empty() || sid.empty() || parentEnd == std::wstring_view::npos)
        return false;
    const auto expected = std::wstring(volume) + LR"($Recycle.Bin\)" + sid;
    return CompareStringOrdinal(normalizedReceipt.data(), static_cast<int>(parentEnd),
                                expected.c_str(), static_cast<int>(expected.size()), TRUE) == CSTR_EQUAL;
}

inline bool ReadShellItemIdentity(IShellItem* item, RecycleFileId& id, std::wstring& normalizedPath)
{
    PWSTR path = nullptr;
    const HRESULT hr = item ? item->GetDisplayName(SIGDN_FILESYSPATH, &path) : E_INVALIDARG;
    const std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)> name(path, CoTaskMemFree);
    return SUCCEEDED(hr) && name && ReadFileId(name.get(), id, &normalizedPath);
}

struct VerifiedSource
{
    RecycleFileId id{};
    std::wstring normalizedPath;

    HRESULT Check(IShellItem* item) const
    {
        try
        {
            RecycleFileId actual{};
            std::wstring actualPath;
            return ReadShellItemIdentity(item, actual, actualPath) && SameFileId(id, actual) &&
                   actualPath == normalizedPath ? S_OK : HRESULT_FROM_WIN32(ERROR_FILE_INVALID);
        }
        catch (const std::bad_alloc&)
        {
            return E_OUTOFMEMORY;
        }
    }
};
}

// The caller retains this sink until it releases IFileOperation.
class OperationRecycleProgress final : public IFileOperationProgressSink
{
public:
    explicit OperationRecycleProgress(bool captureReceipt = false,
                                      const OperationRecycleDetail::VerifiedSource* source = nullptr)
        : captureReceipt(captureReceipt), source(source) {}
    ~OperationRecycleProgress()
    {
        if (receipt)
            receipt->Release();
    }
    OperationRecycleProgress(const OperationRecycleProgress&) = delete;
    OperationRecycleProgress& operator=(const OperationRecycleProgress&) = delete;

    IShellItem* Receipt() const { return receipt; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override
    {
        if (!object)
            return E_POINTER;
        *object = nullptr;
        if (iid != IID_IUnknown && iid != __uuidof(IFileOperationProgressSink))
            return E_NOINTERFACE;
        *object = static_cast<IFileOperationProgressSink*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&references)); }
    ULONG STDMETHODCALLTYPE Release() override { return static_cast<ULONG>(InterlockedDecrement(&references)); }

    HRESULT STDMETHODCALLTYPE PreDeleteItem(DWORD flags, IShellItem* item) override
    {
        if (!(flags & TSF_DELETE_RECYCLE_IF_POSSIBLE))
            failure = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        else if (source && SUCCEEDED(failure))
            failure = source->Check(item);
        return failure;
    }
    HRESULT STDMETHODCALLTYPE PostDeleteItem(DWORD, IShellItem*, HRESULT result, IShellItem* recycled) override
    {
        if (FAILED(failure))
            return failure;
        if (FAILED(result))
            failure = result;
        else if (!recycled)
            failure = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        else
        {
            if (captureReceipt && !completed)
            {
                recycled->AddRef();
                receipt = recycled;
            }
            ++completed;
        }
        return failure;
    }
    HRESULT Result(HRESULT operation, size_t expected) const
    {
        if (FAILED(failure))
            return NormalizeRecycleResult(failure);
        if (FAILED(operation))
            return NormalizeRecycleResult(operation);
        return completed == expected ? S_OK : HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }

    HRESULT STDMETHODCALLTYPE StartOperations() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE FinishOperations(HRESULT) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreRenameItem(DWORD, IShellItem*, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostRenameItem(DWORD, IShellItem*, LPCWSTR, HRESULT, IShellItem*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreMoveItem(DWORD, IShellItem*, IShellItem*, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostMoveItem(DWORD, IShellItem*, IShellItem*, LPCWSTR, HRESULT, IShellItem*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreCopyItem(DWORD, IShellItem*, IShellItem*, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostCopyItem(DWORD, IShellItem*, IShellItem*, LPCWSTR, HRESULT, IShellItem*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreNewItem(DWORD, IShellItem*, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostNewItem(DWORD, IShellItem*, LPCWSTR, LPCWSTR, DWORD, HRESULT, IShellItem*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE UpdateProgress(UINT, UINT) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE ResetTimer() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PauseTimer() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE ResumeTimer() override { return S_OK; }

private:
    bool captureReceipt;
    const OperationRecycleDetail::VerifiedSource* source;
    IShellItem* receipt = nullptr;
    LONG references = 1;
    HRESULT failure = S_OK;
    size_t completed = 0;
};

inline bool IsRecycleDosDevice(std::wstring_view component)
{
    auto stem = component.substr(0, component.find(L'.'));
    while (!stem.empty() && stem.back() == L' ')
        stem.remove_suffix(1);
    const auto equals = [](std::wstring_view name, std::wstring_view reserved) {
        return name.size() == reserved.size() &&
               CompareStringOrdinal(name.data(), static_cast<int>(name.size()),
                                    reserved.data(), static_cast<int>(reserved.size()), TRUE) == CSTR_EQUAL;
    };
    if (equals(stem, L"CON") || equals(stem, L"PRN") || equals(stem, L"AUX") ||
        equals(stem, L"NUL") || equals(stem, L"CONIN$") || equals(stem, L"CONOUT$"))
        return true;
    if (stem.size() != 4 || (!equals(stem.substr(0, 3), L"COM") && !equals(stem.substr(0, 3), L"LPT")))
        return false;
    const wchar_t digit = stem.back();
    return (digit >= L'1' && digit <= L'9') || digit == L'\u00B9' || digit == L'\u00B2' || digit == L'\u00B3';
}

// Remove only the Win32 transport prefix. Reject paths the Shell could
// reinterpret as another object or namespace after that prefix is removed.
inline HRESULT OperationRecyclePath(const std::wstring& exactPath, std::wstring& shellPath)
{
    shellPath = exactPath;
    bool extendedDrive = false;
    if (shellPath.compare(0, 8, LR"(\\?\UNC\)") == 0)
        shellPath = LR"(\\)" + shellPath.substr(8);
    else if (shellPath.compare(0, 4, LR"(\\?\)") == 0)
    {
        extendedDrive = true;
        shellPath.erase(0, 4);
    }
    if (shellPath.empty())
        return E_INVALIDARG;
    const bool drive = shellPath.size() >= 3 &&
        ((shellPath[0] >= L'A' && shellPath[0] <= L'Z') || (shellPath[0] >= L'a' && shellPath[0] <= L'z')) &&
        shellPath[1] == L':' && shellPath[2] == L'\\';
    const bool unc = shellPath.compare(0, 2, LR"(\\)") == 0;
    constexpr HRESULT invalidName = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    if (!drive && (!unc || extendedDrive))
        return invalidName;

    const std::wstring_view path(shellPath);
    size_t start = drive ? 3 : 2;
    size_t count = 0;
    for (;;)
    {
        const size_t end = path.find(L'\\', start);
        const auto part = path.substr(start, end == path.npos ? path.npos : end - start);
        if (part.empty() || part.back() == L'.' || part.back() == L' ' ||
            part.find_first_of(L"<>:\"/|?*") != part.npos ||
            ((drive || count >= 2) && IsRecycleDosDevice(part)))
            return invalidName;
        for (wchar_t ch : part)
            if (ch < L' ')
                return invalidName;
        ++count;
        if (end == path.npos)
            break;
        start = end + 1;
    }
    return unc && count < 3 ? invalidName : S_OK;
}

// Resolve the volume through the same exact wide identity used by the operation.
// The legacy ANSI mount-point traversal cannot identify long Unicode parents.
inline UINT OperationRecycleDriveTypeW(const std::wstring& exactPath)
{
    std::wstring volume(32768, L'\0');
    if (exactPath.empty() || !GetVolumePathNameW(exactPath.c_str(), &volume[0], (DWORD)volume.size()))
        return DRIVE_UNKNOWN;
    return GetDriveTypeW(volume.c_str());
}

// The caller has validated the Shell path and initialized COM.
inline HRESULT RunSingleRecycleOperation(HWND owner, const std::wstring& shellPath,
                                        OperationRecycleProgress& progress, bool showElevation = false)
{
    IFileOperation* operation = NULL;
    IShellItem* item = NULL;
    HRESULT result = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&operation));
    if (SUCCEEDED(result)) result = operation->SetOwnerWindow(owner);
    if (SUCCEEDED(result))
        result = operation->SetOperationFlags(OperationRecycleFlags | FOF_SILENT |
                                              (showElevation ? FOFX_SHOWELEVATIONPROMPT : 0));
    if (SUCCEEDED(result)) result = SHCreateItemFromParsingName(shellPath.c_str(), NULL, IID_PPV_ARGS(&item));
    if (SUCCEEDED(result)) result = operation->DeleteItem(item, &progress);
    if (SUCCEEDED(result)) result = operation->PerformOperations();
    if (SUCCEEDED(result))
    {
        BOOL aborted = TRUE;
        result = operation->GetAnyOperationsAborted(&aborted);
        if (SUCCEEDED(result) && aborted) result = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    result = progress.Result(result, 1);
    if (item != NULL) item->Release();
    if (operation != NULL) operation->Release();
    return result;
}

// One selected-extension worker item, preserving the same Unicode/long-path
// identity as the direct Win32 operation. The caller owns the usual error UI.
inline DWORD RecycleOperationFileW(HWND owner, const std::wstring& exactPath)
{
    std::wstring shellPath;
    HRESULT result = OperationRecyclePath(exactPath, shellPath);
    if (FAILED(result)) return HRESULT_CODE(result);
    const HRESULT initialized = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE)
        return ERROR_GEN_FAILURE;
    OperationRecycleProgress progress;
    result = RunSingleRecycleOperation(owner, shellPath, progress);
    if (SUCCEEDED(initialized)) CoUninitialize();
    if (SUCCEEDED(result)) return ERROR_SUCCESS;
    return HRESULT_FACILITY(result) == FACILITY_WIN32 ? HRESULT_CODE(result) : ERROR_NOT_SUPPORTED;
}
