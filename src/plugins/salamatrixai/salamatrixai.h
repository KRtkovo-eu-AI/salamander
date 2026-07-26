// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "../shared/spl_base.h"
#include "../salamatrix/salamatrix_ai.h"
#include "../salamatrix/salamatrix_runtime_api.h"
#include "../salamatrix/salamatrix_script_runner.h"
#include "../salamatrix/salamatrix_ui.h"

extern HINSTANCE DLLInstance;
extern CSalamanderGeneralAbstract* SalamanderGeneral;

#define IDS_AI_ASSISTANT_MENU 1000

class CLocalAssistantProvider : public Salamatrix::AI::IAssistantProvider
{
private:
    Salamatrix::AI::AssistantProviderDescriptor m_descriptor;
    mutable std::wstring m_commandLine;

    void ResolveCommand() const;

public:
    CLocalAssistantProvider();
    virtual const Salamatrix::AI::AssistantProviderDescriptor* WINAPI GetDescriptor() const;
    virtual BOOL WINAPI IsAvailable() const;
    virtual BOOL WINAPI Generate(const Salamatrix::AI::AssistantRequest* request,
                                 Salamatrix::AI::AssistantResponse* response);
};

// Optional dependency-free local model provider. It speaks either Ollama's
// /api/generate protocol or an OpenAI-compatible chat-completions protocol
// (including llama.cpp's local server) over WinHTTP. The endpoint, protocol,
// and model are selected through SALAMATRIX_AI_* environment settings.
class CLocalHttpAssistantProvider : public Salamatrix::AI::IAssistantProvider
{
private:
    Salamatrix::AI::AssistantProviderDescriptor m_descriptor;
    mutable std::wstring m_url;
    mutable std::wstring m_model;
    mutable std::wstring m_protocol;

    void ResolveConfiguration() const;

public:
    CLocalHttpAssistantProvider();
    virtual const Salamatrix::AI::AssistantProviderDescriptor* WINAPI GetDescriptor() const;
    virtual BOOL WINAPI IsAvailable() const;
    virtual BOOL WINAPI Generate(const Salamatrix::AI::AssistantRequest* request,
                                 Salamatrix::AI::AssistantResponse* response);
};

class CAIPluginMenuExt : public CPluginInterfaceForMenuExtAbstract
{
public:
    enum { CmdOpenAssistant = 1 };
    virtual DWORD WINAPI GetMenuItemState(int id, DWORD eventMask);
    virtual BOOL WINAPI ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander,
                                        HWND parent, int id, DWORD eventMask);
    virtual BOOL WINAPI HelpForMenuItem(HWND parent, int id);
    virtual void WINAPI BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* salamander);
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
    virtual void WINAPI Configuration(HWND parent) {}
    virtual void WINAPI Connect(HWND parent, CSalamanderConnectAbstract* salamander);
    virtual void WINAPI ReleasePluginDataInterface(CPluginDataInterfaceAbstract*) {}
    virtual CPluginInterfaceForArchiverAbstract* WINAPI GetInterfaceForArchiver() { return NULL; }
    virtual CPluginInterfaceForViewerAbstract* WINAPI GetInterfaceForViewer() { return NULL; }
    virtual CPluginInterfaceForMenuExtAbstract* WINAPI GetInterfaceForMenuExt();
    virtual CPluginInterfaceForFSAbstract* WINAPI GetInterfaceForFS() { return NULL; }
    virtual CPluginInterfaceForThumbLoaderAbstract* WINAPI GetInterfaceForThumbLoader() { return NULL; }
    virtual void WINAPI Event(int event, DWORD param);
    virtual void WINAPI ClearHistory(HWND parent) {}
    virtual void WINAPI AcceptChangeOnPathNotification(const char*, BOOL) {}
    virtual void WINAPI PasswordManagerEvent(HWND parent, int event) {}
};

extern CPluginInterface PluginInterface;
