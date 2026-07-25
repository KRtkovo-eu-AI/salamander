// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Automation Plugin for Open Salamander

    salamatrixbridge.h
    Thin Automation-side consumer bridge for the Salamatrix runtime plugin.
*/

#pragma once

#include <string>

#include "../salamatrix/salamatrix_automation.h"
#include "../salamatrix/salamatrix_ai.h"
#include "../salamatrix/salamatrix_events.h"
#include "../salamatrix/salamatrix_extensions.h"
#include "../salamatrix/salamatrix_runtime_api.h"
#include "../salamatrix/salamatrix_sides.h"
#include "../salamatrix/salamatrix_storage.h"

class CAutomationActiveScriptRuntimeAdapter : public Salamatrix::Runtime::IRuntimeAdapter
{
private:
    Salamatrix::Runtime::RuntimeAdapterDescriptor m_oDescriptor;
    PCTSTR m_pszFileExtension;
    CLSID m_clsidEngine;

public:
    CAutomationActiveScriptRuntimeAdapter(
        const char* runtimeId,
        const char* displayName,
        const char* languageId,
        const char* fileExtension,
        PCTSTR nativeFileExtension,
        const CLSID& engineClsid);

    virtual const Salamatrix::Runtime::RuntimeAdapterDescriptor* WINAPI GetDescriptor() const;
    virtual BOOL WINAPI IsAvailable() const;
    virtual BOOL WINAPI SupportsEntryPoint(const char* entryPoint) const;
    virtual BOOL WINAPI Execute(
        const Salamatrix::Runtime::RuntimeExecutionRequest* request,
        Salamatrix::Runtime::RuntimeExecutionResult* result);
};

/// Executes a script through a deliberately small, out-of-process CLI
/// contract. The host owns process creation, timeout, exit status, and bounded
/// stdout/stderr capture; the runtime itself never receives raw Salamander
/// pointers. This transitional implementation remains for compatibility tests;
/// standalone Python, PowerShell, PHP, and JavaScript provider plugins own the
/// modern broker registrations.
class CAutomationProcessRuntimeAdapter : public Salamatrix::Runtime::IRuntimeAdapter
{
public:
    enum ProcessKind
    {
        ProcessKindPython,
        ProcessKindPowerShell,
        ProcessKindPhp
    };

private:
    Salamatrix::Runtime::RuntimeAdapterDescriptor m_oDescriptor;
    const char* m_pszExtension;
    const wchar_t* m_pszEnvironmentVariable;
    const wchar_t* m_pszCandidateOne;
    const wchar_t* m_pszCandidateTwo;
    ProcessKind m_kind;
    mutable std::wstring m_executablePath;

    void ResolveInterpreter() const;

public:
    CAutomationProcessRuntimeAdapter(
        const char* runtimeId,
        const char* displayName,
        const char* languageId,
        const char* fileExtension,
        const wchar_t* environmentVariable,
        const wchar_t* candidateOne,
        const wchar_t* candidateTwo,
        ProcessKind kind);

    virtual const Salamatrix::Runtime::RuntimeAdapterDescriptor* WINAPI GetDescriptor() const;
    virtual BOOL WINAPI IsAvailable() const;
    virtual BOOL WINAPI SupportsEntryPoint(const char* entryPoint) const;
    virtual BOOL WINAPI Execute(
        const Salamatrix::Runtime::RuntimeExecutionRequest* request,
        Salamatrix::Runtime::RuntimeExecutionResult* result);
    virtual BOOL WINAPI StartPersistent(
        const Salamatrix::Runtime::RuntimeExecutionRequest* request,
        Salamatrix::Runtime::IRuntimeSession** session);
};

/// Caches host-registered Salamatrix services for the Automation plugin.
///
/// The bridge is intentionally consumer-only: it never creates a fallback
/// Salamatrix runtime and does not duplicate UI/command implementations. If the
/// Salamatrix runtime plugin is installed and loaded, Automation can use the
/// host service registry; otherwise callers can keep using legacy Automation
/// objects and future script wrappers can report a clear missing-runtime error.
class CAutomationSalamatrixBridge
{
private:
    bool m_bQueried;
    CSalamanderGeneralAbstract* m_pGeneral;
    Salamatrix::Automation::ScriptRootAdapter* m_pScriptRoot;
    Salamatrix::UI::IUIService* m_pUIService;
    Salamatrix::Commands::ICommandService* m_pCommandService;
    Salamatrix::FileOperations::IFileOperationsService* m_pFileOperationsService;
    Salamatrix::Runtime::IRuntimeService* m_pRuntimeService;
    Salamatrix::Sides::ISidesService* m_pSidesService;
    Salamatrix::Events::IEventsService* m_pEventsService;
    Salamatrix::Extensions::IExtensionsService* m_pExtensionsService;
    Salamatrix::Storage::IStorageService* m_pStorageService;
    Salamatrix::AI::IAssistantService* m_pAssistantService;
    DWORD m_dwAutomationVersion;
    DWORD m_dwUIVersion;
    DWORD m_dwCommandsVersion;
    DWORD m_dwFileOperationsVersion;
    DWORD m_dwRuntimeVersion;
    DWORD m_dwSidesVersion;
    DWORD m_dwEventsVersion;
    DWORD m_dwExtensionsVersion;
    DWORD m_dwStorageVersion;
    DWORD m_dwAssistantVersion;
    CAutomationActiveScriptRuntimeAdapter m_oJScriptRuntime;
    CAutomationActiveScriptRuntimeAdapter m_oVBScriptRuntime;
    CAutomationActiveScriptRuntimeAdapter m_oPythonRuntime;
    CAutomationActiveScriptRuntimeAdapter m_oPHPRuntime;
    // Transitional implementations retained for direct process-runtime tests;
    // modern adapters are registered by standalone runtime .SPL providers.
    CAutomationProcessRuntimeAdapter m_oCPythonRuntime;
    CAutomationProcessRuntimeAdapter m_oPowerShellRuntime;
    CAutomationProcessRuntimeAdapter m_oPHPCliRuntime;
    bool m_bRuntimeAdaptersRegistered;

    static void* QueryService(
        CSalamanderGeneralAbstract* salamander,
        const char* serviceName,
        DWORD minVersion,
        DWORD* actualVersion);
    void RegisterRuntimeAdapters();
    void UnregisterRuntimeAdapters();

public:
    CAutomationSalamatrixBridge();

    void Reset();
    void Refresh(CSalamanderGeneralAbstract* salamander);

    bool WasQueried() const { return m_bQueried; }
    bool IsAvailable() const { return m_pScriptRoot != NULL; }
    bool HasUI() const { return m_pUIService != NULL; }
    bool HasCommands() const { return m_pCommandService != NULL; }
    bool HasFileOperations() const { return m_pFileOperationsService != NULL; }
    bool HasRuntimeBroker() const { return m_pRuntimeService != NULL; }
    bool HasSides() const { return m_pSidesService != NULL; }
    bool HasEvents() const { return m_pEventsService != NULL; }
    bool HasExtensions() const { return m_pExtensionsService != NULL; }
    bool HasStorage() const { return m_pStorageService != NULL; }
    bool HasAssistant() const { return m_pAssistantService != NULL; }

    Salamatrix::Automation::ScriptRootAdapter* GetScriptRoot() const { return m_pScriptRoot; }
    Salamatrix::UI::IUIService* GetUIService() const { return m_pUIService; }
    Salamatrix::Commands::ICommandService* GetCommandService() const { return m_pCommandService; }
    Salamatrix::FileOperations::IFileOperationsService* GetFileOperationsService() const { return m_pFileOperationsService; }
    Salamatrix::Runtime::IRuntimeService* GetRuntimeService() const { return m_pRuntimeService; }
    Salamatrix::Sides::ISidesService* GetSidesService() const { return m_pSidesService; }
    Salamatrix::Events::IEventsService* GetEventsService() const { return m_pEventsService; }
    Salamatrix::Extensions::IExtensionsService* GetExtensionsService() const { return m_pExtensionsService; }
    Salamatrix::Storage::IStorageService* GetStorageService() const { return m_pStorageService; }
    Salamatrix::AI::IAssistantService* GetAssistantService() const { return m_pAssistantService; }

    void GetStatusText(PTSTR buffer, int cchBuffer) const;
};
