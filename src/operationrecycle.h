// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <string>

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
    IFileOperation* operation = NULL;
    IShellItem* item = NULL;
    result = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&operation));
    if (SUCCEEDED(result)) result = operation->SetOwnerWindow(owner);
    if (SUCCEEDED(result))
        result = operation->SetOperationFlags(FOF_ALLOWUNDO | FOFX_RECYCLEONDELETE |
            FOFX_ADDUNDORECORD | FOFX_EARLYFAILURE | FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI);
    if (SUCCEEDED(result)) result = SHCreateItemFromParsingName(shellPath.c_str(), NULL, IID_PPV_ARGS(&item));
    if (SUCCEEDED(result)) result = operation->DeleteItem(item, NULL);
    if (SUCCEEDED(result)) result = operation->PerformOperations();
    if (SUCCEEDED(result))
    {
        BOOL aborted = TRUE;
        result = operation->GetAnyOperationsAborted(&aborted);
        if (SUCCEEDED(result) && aborted) result = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    if (item != NULL) item->Release();
    if (operation != NULL) operation->Release();
    if (SUCCEEDED(initialized)) CoUninitialize();
    if (SUCCEEDED(result)) return ERROR_SUCCESS;
    return HRESULT_FACILITY(result) == FACILITY_WIN32 ? HRESULT_CODE(result) : ERROR_GEN_FAILURE;
}
