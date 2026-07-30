// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

extern HINSTANCE DLLInstance;
extern CSalamanderGeneralAbstract* SalamanderGeneral;
extern CSalamanderGUIAbstract* SalamanderGUI;

#define IDS_LLAMA_TITLE 1100
#define IDS_LLAMA_UI_UNAVAILABLE 1101
#define IDS_LLAMA_MODEL_LABEL 1102
#define IDS_LLAMA_MODEL_15B 1103
#define IDS_LLAMA_MODEL_05B 1104
#define IDS_LLAMA_STATUS_READY 1105
#define IDS_LLAMA_STATUS_MODEL_MISSING 1106
#define IDS_LLAMA_STATUS_RUNTIME_MISSING 1107
#define IDS_LLAMA_DOWNLOAD 1108
#define IDS_LLAMA_REFRESH 1109
#define IDS_LLAMA_OPEN_FOLDER 1110
#define IDS_LLAMA_CLOSE 1111
#define IDS_LLAMA_DOWNLOAD_STARTED 1112
#define IDS_LLAMA_DOWNLOAD_FAILED 1113
#define IDS_LLAMA_ABOUT 1114

enum LocalLlamaModel
{
    LocalLlamaModelQwen15B = 0,
    LocalLlamaModelQwen05B = 1
};

LocalLlamaModel GetSelectedLocalLlamaModel();
const wchar_t* GetSelectedLocalLlamaModelFileName();

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
