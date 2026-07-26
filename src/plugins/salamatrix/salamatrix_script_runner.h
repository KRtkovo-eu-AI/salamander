// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

class CSalamanderForOperationsAbstract;

namespace Salamatrix
{
namespace Automation
{

#define SALAMATRIX_SERVICE_SCRIPT_RUNNER "Salamatrix.ScriptRunner"
#define SALAMATRIX_SCRIPT_RUNNER_VERSION_1_0 0x00010000

/// Request used by native clients (notably SalamatrixAI.SPL) to run a saved
/// script through the same Automation host dispatcher as a manifest extension.
/// Paths are wide so Unicode and extended-length Win32 paths are preserved.
struct GeneratedScriptRequest
{
    DWORD StructSize;
    const wchar_t* EntryPoint;
    const char* RuntimeId;
    const char* ExtensionId;
    HWND ParentWindow;
    DWORD TimeoutMs;
    /// Optional Salamander operation context used by shared progress dialogs.
    /// Appended to preserve the offsets of the original request fields. The
    /// pointer is borrowed and must remain valid for ExecuteGenerated().
    CSalamanderForOperationsAbstract* Operation;

    GeneratedScriptRequest()
        : StructSize(sizeof(GeneratedScriptRequest)),
          EntryPoint(NULL),
          RuntimeId(NULL),
          ExtensionId(NULL),
          ParentWindow(NULL),
          TimeoutMs(120000),
          Operation(NULL)
    {
    }
};

struct GeneratedScriptResult
{
    DWORD StructSize;
    BOOL Succeeded;
    HRESULT ErrorCode;
    DWORD ExitCode;
    wchar_t Message[512];

    GeneratedScriptResult()
        : StructSize(sizeof(GeneratedScriptResult)),
          Succeeded(FALSE),
          ErrorCode(S_OK),
          ExitCode(0)
    {
        Message[0] = L'\0';
    }
};

class IScriptRunner
{
public:
    virtual DWORD WINAPI GetVersion() const = 0;
    virtual BOOL WINAPI ExecuteGenerated(
        const GeneratedScriptRequest* request,
        GeneratedScriptResult* result) = 0;

    /// Refreshes the existing Plugin Manager manifest/script discovery when
    /// a native helper writes a new extension package. Appended so older
    /// runners remain ABI-compatible.
    virtual BOOL WINAPI RefreshExtensions()
    {
        return FALSE;
    }

protected:
    virtual ~IScriptRunner() {}
};

} // namespace Automation
} // namespace Salamatrix
