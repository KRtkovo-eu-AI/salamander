// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "salamatrixrunner.h"
#include "scriptlist.h"

DWORD WINAPI CGeneratedScriptRunner::GetVersion() const
{
    return SALAMATRIX_SCRIPT_RUNNER_VERSION_1_0;
}

BOOL WINAPI CGeneratedScriptRunner::ExecuteGenerated(
    const Salamatrix::Automation::GeneratedScriptRequest* request,
    Salamatrix::Automation::GeneratedScriptResult* result)
{
    if (result == NULL || result->StructSize < sizeof(*result))
        return FALSE;
    *result = Salamatrix::Automation::GeneratedScriptResult();
    const DWORD minimumRequestSize =
        static_cast<DWORD>(offsetof(Salamatrix::Automation::GeneratedScriptRequest, TimeoutMs) +
                           sizeof(DWORD));
    if (request == NULL || request->StructSize < minimumRequestSize ||
        request->EntryPoint == NULL || request->RuntimeId == NULL ||
        request->ExtensionId == NULL)
    {
        result->ErrorCode = E_INVALIDARG;
        StringCchCopyW(result->Message, _countof(result->Message),
                       L"The generated script request is invalid.");
        return FALSE;
    }

    std::wstring path(request->EntryPoint);
    if (path.empty() || path.size() >= SAL_MAX_PATH)
    {
        result->ErrorCode = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
        StringCchCopyW(result->Message, _countof(result->Message),
                       L"The generated script path is too long.");
        return FALSE;
    }

#ifdef UNICODE
    std::vector<TCHAR> nativePath(path.begin(), path.end());
    nativePath.push_back(L'\0');
#else
    int nativeLength = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS,
                                           path.c_str(), -1, NULL, 0, NULL, NULL);
    if (nativeLength <= 0)
        return FALSE;
    std::vector<TCHAR> nativePath(static_cast<size_t>(nativeLength));
    if (WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, path.c_str(), -1,
                            &nativePath[0], nativeLength, NULL, NULL) <= 0)
        return FALSE;
#endif

    CScriptContainer container;
    CScriptInfo script(&nativePath[0], &container);
    if (!script.ConfigureGeneratedRuntime(request->RuntimeId,
                                          request->ExtensionId))
    {
        result->ErrorCode = E_INVALIDARG;
        StringCchCopyW(result->Message, _countof(result->Message),
                       L"The generated script runtime is invalid.");
        return FALSE;
    }

    CScriptInfo::EXECUTION_INFO execution;
    const DWORD operationOffset =
        static_cast<DWORD>(offsetof(Salamatrix::Automation::GeneratedScriptRequest, Operation) +
                           sizeof(CSalamanderForOperationsAbstract*));
    execution.pOperation = request->StructSize >= operationOffset
                               ? request->Operation
                               : NULL;
    BOOL succeeded = script.Execute(execution) ? TRUE : FALSE;
    result->Succeeded = succeeded;
    result->ErrorCode = succeeded ? S_OK : E_FAIL;
    result->ExitCode = succeeded ? 0 : 1;
    if (!succeeded)
        StringCchCopyW(result->Message, _countof(result->Message),
                       L"The generated script failed in the Automation host.");
    return succeeded;
}
