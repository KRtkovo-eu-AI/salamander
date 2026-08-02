// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "../shared/spl_base.h"
#include "../salamatrix/salamatrix_runtime_api.h"

extern HINSTANCE DLLInstance;
extern CSalamanderGeneralAbstract* SalamanderGeneral;

class CPHPRuntimeAdapter : public Salamatrix::Runtime::IRuntimeAdapter
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
    mutable bool m_bInterpreterResolved;

    void ResolveInterpreter() const;

public:
    CPHPRuntimeAdapter(
        const char* runtimeId,
        const char* displayName,
        const char* languageId,
        const char* fileExtension,
        const wchar_t* environmentVariable,
        const wchar_t* candidateOne,
        const wchar_t* candidateTwo,
        ProcessKind kind);

    virtual const Salamatrix::Runtime::RuntimeAdapterDescriptor* WINAPI
    GetDescriptor() const;
    virtual BOOL WINAPI IsAvailable() const;
    virtual BOOL WINAPI SupportsEntryPoint(const char* entryPoint) const;
    virtual BOOL WINAPI Execute(
        const Salamatrix::Runtime::RuntimeExecutionRequest* request,
        Salamatrix::Runtime::RuntimeExecutionResult* result);
    virtual BOOL WINAPI StartPersistent(
        const Salamatrix::Runtime::RuntimeExecutionRequest* request,
        Salamatrix::Runtime::IRuntimeSession** session);
    const std::wstring& GetExecutablePath() const;
    void InvalidateExecutablePath();
};

class CPluginInterface : public CPluginInterfaceAbstract
{
public:
    virtual void WINAPI About(HWND parent);
    virtual BOOL WINAPI Release(HWND parent, BOOL force);
    virtual void WINAPI LoadConfiguration(HWND parent, HKEY regKey,
                                          CSalamanderRegistryAbstract* registry);
    virtual void WINAPI SaveConfiguration(HWND parent, HKEY regKey,
                                           CSalamanderRegistryAbstract* registry);
    virtual void WINAPI Configuration(HWND parent);
    virtual void WINAPI Connect(HWND parent, CSalamanderConnectAbstract* salamander);
    virtual void WINAPI ReleasePluginDataInterface(CPluginDataInterfaceAbstract*) {}
    virtual CPluginInterfaceForArchiverAbstract* WINAPI GetInterfaceForArchiver() { return NULL; }
    virtual CPluginInterfaceForViewerAbstract* WINAPI GetInterfaceForViewer() { return NULL; }
    virtual CPluginInterfaceForMenuExtAbstract* WINAPI GetInterfaceForMenuExt() { return NULL; }
    virtual CPluginInterfaceForFSAbstract* WINAPI GetInterfaceForFS() { return NULL; }
    virtual CPluginInterfaceForThumbLoaderAbstract* WINAPI GetInterfaceForThumbLoader() { return NULL; }
    virtual void WINAPI Event(int event, DWORD param);
    virtual void WINAPI ClearHistory(HWND parent) {}
    virtual void WINAPI AcceptChangeOnPathNotification(const char*, BOOL) {}
    virtual void WINAPI PasswordManagerEvent(HWND parent, int event) {}
};

extern CPluginInterface PluginInterface;


