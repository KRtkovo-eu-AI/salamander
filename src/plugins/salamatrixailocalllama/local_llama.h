// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

extern HINSTANCE DLLInstance;
extern CSalamanderGeneralAbstract* SalamanderGeneral;
extern CSalamanderGUIAbstract* SalamanderGUI;

class CLocalBundledAssistantProvider : public Salamatrix::AI::IAssistantProvider
{
private:
    Salamatrix::AI::AssistantProviderDescriptor m_descriptor;
    mutable std::wstring m_command;
    mutable std::wstring m_model;
    void ResolveConfiguration() const;

public:
    CLocalBundledAssistantProvider();
    virtual const Salamatrix::AI::AssistantProviderDescriptor* WINAPI GetDescriptor() const;
    virtual BOOL WINAPI IsAvailable() const;
    virtual BOOL WINAPI Generate(const Salamatrix::AI::AssistantRequest* request,
                                 Salamatrix::AI::AssistantResponse* response);
};

class CLocalLlamaPluginInterface : public CPluginInterfaceAbstract
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
    virtual void WINAPI ClearHistory(HWND parent) { UNREFERENCED_PARAMETER(parent); }
    virtual void WINAPI AcceptChangeOnPathNotification(const char*, BOOL) {}
    virtual void WINAPI PasswordManagerEvent(HWND parent, int event)
    { UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(event); }
};

extern CLocalLlamaPluginInterface LocalLlamaPluginInterface;
