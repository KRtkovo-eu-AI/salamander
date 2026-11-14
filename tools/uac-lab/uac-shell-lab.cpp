/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-07 | Probe Shell folder creation with and without an elevation prompt. | Compare IFileOperation with the P0 broker bootstrap. | | GPT-6.1 Sol (Codex) |
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <wrl/client.h>
#include <cstdio>
#include <cwchar>
#include <string>

using Microsoft::WRL::ComPtr;

namespace
{
class Progress final : public IFileOperationProgressSink
{
public:
    HRESULT itemResult = E_PENDING;
    bool sawItem = false;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override
    {
        *out = nullptr;
        if (id != IID_IUnknown && id != IID_IFileOperationProgressSink)
            return E_NOINTERFACE;
        *out = static_cast<IFileOperationProgressSink*>(this);
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG remaining = --refs;
        if (!remaining)
            delete this;
        return remaining;
    }

    HRESULT STDMETHODCALLTYPE StartOperations() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE FinishOperations(HRESULT) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreRenameItem(DWORD, IShellItem*, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostRenameItem(DWORD, IShellItem*, LPCWSTR, HRESULT, IShellItem*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreMoveItem(DWORD, IShellItem*, IShellItem*, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostMoveItem(DWORD, IShellItem*, IShellItem*, LPCWSTR, HRESULT, IShellItem*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreCopyItem(DWORD, IShellItem*, IShellItem*, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostCopyItem(DWORD, IShellItem*, IShellItem*, LPCWSTR, HRESULT, IShellItem*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreDeleteItem(DWORD, IShellItem*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostDeleteItem(DWORD, IShellItem*, HRESULT, IShellItem*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreNewItem(DWORD, IShellItem*, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostNewItem(DWORD, IShellItem*, LPCWSTR, LPCWSTR, DWORD, HRESULT result, IShellItem*) override
    {
        sawItem = true;
        itemResult = result;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE UpdateProgress(UINT, UINT) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE ResetTimer() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PauseTimer() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE ResumeTimer() override { return S_OK; }

private:
    ULONG refs = 1; // STA probe.
};

int Fail(const wchar_t* step, HRESULT result)
{
    std::fwprintf(stderr, L"%ls failed (0x%08lX)\n", step, static_cast<unsigned long>(result));
    return 1;
}

int Probe(const wchar_t* prefix, bool prompt)
{
    PWSTR programFiles = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, nullptr, &programFiles);
    if (FAILED(hr))
        return Fail(L"Locate Program Files", hr);
    const std::wstring parent = std::wstring(programFiles) + L"\\Samandarin-UacLab";
    CoTaskMemFree(programFiles);
    const DWORD parentAttrs = GetFileAttributesW(parent.c_str());
    if (parentAttrs == INVALID_FILE_ATTRIBUTES || !(parentAttrs & FILE_ATTRIBUTE_DIRECTORY) ||
        (parentAttrs & FILE_ATTRIBUTE_REPARSE_POINT))
        return Fail(L"Check lab parent", E_ACCESSDENIED);

    GUID id{};
    wchar_t guid[39]{};
    if (FAILED(CoCreateGuid(&id)) || !StringFromGUID2(id, guid, ARRAYSIZE(guid)))
        return Fail(L"Generate unique test name", E_FAIL);
    const std::wstring leaf = std::wstring(prefix) + L"-" + guid;
    const std::wstring target = parent + L"\\" + leaf;
    std::wprintf(L"Target=%ls\n", leaf.c_str());
    if (GetFileAttributesW(target.c_str()) != INVALID_FILE_ATTRIBUTES || GetLastError() != ERROR_FILE_NOT_FOUND)
        return Fail(L"Target must be absent", HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));

    // Baseline uses the same parent and name. If the caller can write, this is
    // the wrong fixture/token, so remove only our newly created empty leaf.
    if (CreateDirectoryW(target.c_str(), nullptr))
    {
        if (!RemoveDirectoryW(target.c_str()))
            return Fail(L"Remove baseline leaf", HRESULT_FROM_WIN32(GetLastError()));
        return Fail(L"Expected a protected target and a non-elevated caller", E_ACCESSDENIED);
    }
    const DWORD nativeError = GetLastError();
    std::wprintf(L"CreateDirectoryW=%lu\n", nativeError);
    if (nativeError != ERROR_ACCESS_DENIED)
        return Fail(L"Native baseline", HRESULT_FROM_WIN32(nativeError));

    ComPtr<IFileOperation> operation;
    hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&operation));
    if (FAILED(hr))
        return Fail(L"Create IFileOperation", hr);
    const DWORD flags = FOF_NOERRORUI | FOF_SILENT | FOFX_EARLYFAILURE |
                        (prompt ? FOFX_SHOWELEVATIONPROMPT | FOFX_REQUIREELEVATION : 0);
    hr = operation->SetOperationFlags(flags);
    if (FAILED(hr))
        return Fail(L"Set Shell flags", hr);
    if (const HWND owner = GetConsoleWindow())
    {
        hr = operation->SetOwnerWindow(owner);
        if (FAILED(hr))
            return Fail(L"Set Shell owner", hr);
    }
    ComPtr<IShellItem> folder;
    hr = SHCreateItemFromParsingName(parent.c_str(), nullptr, IID_PPV_ARGS(&folder));
    if (FAILED(hr))
        return Fail(L"Resolve Shell parent", hr);

    ComPtr<Progress> sink;
    sink.Attach(new Progress);
    const HRESULT queued = operation->NewItem(folder.Get(), FILE_ATTRIBUTE_DIRECTORY, leaf.c_str(), nullptr, sink.Get());
    if (FAILED(queued))
        return Fail(L"Queue NewItem", queued);
    const HRESULT performed = operation->PerformOperations();
    BOOL aborted = FALSE;
    const HRESULT abortedResult = operation->GetAnyOperationsAborted(&aborted);
    const DWORD attrs = GetFileAttributesW(target.c_str());
    const DWORD observeError = attrs == INVALID_FILE_ATTRIBUTES ? GetLastError() : ERROR_SUCCESS;
    std::wprintf(L"Queue=0x%08lX Perform=0x%08lX AbortedHR=0x%08lX Aborted=%d "
                 L"PostNewItem=%ls Item=0x%08lX TargetDirectory=%d ObserveError=%lu\n",
                 static_cast<unsigned long>(queued), static_cast<unsigned long>(performed),
                 static_cast<unsigned long>(abortedResult), aborted, sink->sawItem ? L"yes" : L"no",
                 static_cast<unsigned long>(sink->itemResult),
                 attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY), observeError);
    return 0; // Report data for interpretation even for a canceled operation.
}
}

int wmain(int argc, wchar_t** argv)
{
    const wchar_t* leaf = nullptr;
    bool prompt = true;
    if (argc == 2)
    {
        if (!std::wcscmp(argv[1], L"--control"))
        {
            leaf = L"P0-Shell-Control";
            prompt = false;
        }
        else if (!std::wcscmp(argv[1], L"--approve"))
            leaf = L"P0-Shell-Approve";
        else if (!std::wcscmp(argv[1], L"--cancel"))
            leaf = L"P0-Shell-Cancel";
    }
    if (!leaf)
        return Fail(L"Use --control, --approve or --cancel", E_INVALIDARG);

    const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
        return Fail(L"Initialize COM", hr);
    const int result = Probe(leaf, prompt);
    CoUninitialize();
    return result;
}
