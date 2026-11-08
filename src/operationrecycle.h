// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Allow an exact Shell recycle receipt to be inspected. | Verify account and identity without changing ordinary recycling. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Guard Windows Shell recycling against permanent fallback. | Require recycle intent and a per-item Recycle Bin receipt. | | GPT-6.1 Sol (Codex) |
#pragma once
#include <windows.h>
#include <shobjidl.h>
#include <sherrors.h>
#include <shlobj.h>
#include <shellapi.h>
#include <string>

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

// The caller retains this sink until it releases IFileOperation.
class OperationRecycleProgress final : public IFileOperationProgressSink
{
public:
    explicit OperationRecycleProgress(bool captureReceipt = false) : captureReceipt(captureReceipt) {}
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

    HRESULT STDMETHODCALLTYPE PreDeleteItem(DWORD flags, IShellItem*) override
    {
        if (!(flags & TSF_DELETE_RECYCLE_IF_POSSIBLE))
            failure = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
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
    IShellItem* receipt = nullptr;
    LONG references = 1;
    HRESULT failure = S_OK;
    size_t completed = 0;
};

// The shell does not accept the Win32 extended prefix and may normalize final
// dots/spaces to another file. Remove only that transport prefix, then reject
// ambiguous components before any shell operation can be queued.
inline HRESULT OperationRecyclePath(const std::wstring& exactPath, std::wstring& shellPath)
{
    shellPath = exactPath;
    if (shellPath.compare(0, 8, L"\\\\?\\UNC\\") == 0)
        shellPath = L"\\\\" + shellPath.substr(8);
    else if (shellPath.compare(0, 4, L"\\\\?\\") == 0)
        shellPath.erase(0, 4);
    if (shellPath.empty()) return E_INVALIDARG;
    for (size_t i = 0; i < shellPath.size(); ++i)
        if ((i + 1 == shellPath.size() || shellPath[i + 1] == L'\\' || shellPath[i + 1] == L'/') &&
            (shellPath[i] == L'.' || shellPath[i] <= L' '))
            return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    return S_OK;
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
    IFileOperation* operation = NULL;
    IShellItem* item = NULL;
    result = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&operation));
    if (SUCCEEDED(result)) result = operation->SetOwnerWindow(owner);
    if (SUCCEEDED(result))
        result = operation->SetOperationFlags(OperationRecycleFlags | FOF_SILENT);
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
    if (SUCCEEDED(initialized)) CoUninitialize();
    if (SUCCEEDED(result)) return ERROR_SUCCESS;
    return HRESULT_FACILITY(result) == FACILITY_WIN32 ? HRESULT_CODE(result) : ERROR_NOT_SUPPORTED;
}
