// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

// The provider implementation is shared source with the former in-process
// prototype, but is linked only by this optional companion plug-in. The main
// SalamatrixAI plug-in therefore remains model-free and can be installed alone.
#include "../salamatrixai/bundledprovider.cpp"

HINSTANCE DLLInstance = NULL;
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderGUIAbstract* SalamanderGUI = NULL;
CSalamanderDebugAbstract* SalamanderDebug = NULL;
int SalamanderVersion = 0;
CLocalLlamaPluginInterface LocalLlamaPluginInterface;

namespace
{
CLocalBundledAssistantProvider g_provider;
Salamatrix::AI::IAssistantService* g_ai = NULL;
bool g_registered = false;
bool g_released = false;

static void* Query(const char* serviceId, DWORD minimumVersion)
{
    if (SalamanderGeneral == NULL)
        return NULL;
    CSalamanderServiceQuery query = {};
    query.ServiceId = serviceId;
    query.MinimumVersion = minimumVersion;
    CSalamanderServiceResult result = {};
    return SalamanderGeneral->QueryService(&query, &result) ? result.Interface : NULL;
}

static void EnsureProvider()
{
    if (g_released || g_registered)
        return;
    g_ai = static_cast<Salamatrix::AI::IAssistantService*>(
        Query(SALAMATRIX_SERVICE_AI, SALAMATRIX_AI_VERSION_1_0));
    if (g_ai != NULL) {
        g_registered = g_ai->RegisterProvider(&g_provider) != FALSE;
    }
}
}

void WINAPI CLocalLlamaPluginInterface::About(HWND parent)
{
    SalamanderGeneral->SalMessageBox(
        parent,
        "Optional server-free llama.cpp model provider for Salamatrix AI.",
        "SalamatrixAI Local Llama",
        MB_OK | MB_ICONINFORMATION);
}

BOOL WINAPI CLocalLlamaPluginInterface::Release(HWND parent, BOOL force)
{
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(force);
    if (!g_released && g_registered && g_ai != NULL && SalamanderGeneral != NULL) {
        CSalamanderServiceQuery query = {};
        query.ServiceId = SALAMATRIX_SERVICE_AI;
        query.MinimumVersion = SALAMATRIX_AI_VERSION_1_0;
        CSalamanderServiceResult result = {};
        if (SalamanderGeneral->QueryService(&query, &result) && result.Interface == g_ai)
            g_ai->UnregisterProvider(&g_provider);
    }
    g_registered = false;
    g_ai = NULL;
    g_released = true;
    return TRUE;
}

void WINAPI CLocalLlamaPluginInterface::LoadConfiguration(HWND parent, HKEY regKey,
                                                  CSalamanderRegistryAbstract* registry)
{
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(regKey);
    UNREFERENCED_PARAMETER(registry);
}

void WINAPI CLocalLlamaPluginInterface::SaveConfiguration(HWND parent, HKEY regKey,
                                                 CSalamanderRegistryAbstract* registry)
{
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(regKey);
    UNREFERENCED_PARAMETER(registry);
}

void WINAPI CLocalLlamaPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    UNREFERENCED_PARAMETER(parent);
    EnsureProvider();
    if (SalamanderGUI == NULL || salamander == NULL)
        return;
    CGUIIconListAbstract* iconList = SalamanderGUI->CreateIconList();
    if (iconList == NULL || !iconList->Create(16, 16, 1)) {
        if (iconList != NULL) SalamanderGUI->DestroyIconList(iconList);
        return;
    }
    const UINT loadFlags = SalamanderGeneral != NULL ?
        SalamanderGeneral->GetIconLRFlags() : LR_DEFAULTCOLOR;
    HICON icon = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_PLUGINICON),
                                  IMAGE_ICON, 16, 16, loadFlags);
    if (icon != NULL) {
        iconList->ReplaceIcon(0, icon);
        DestroyIcon(icon);
        salamander->SetIconListForGUI(iconList);
        salamander->SetPluginIcon(0);
        salamander->SetPluginMenuAndToolbarIcon(0);
        iconList = NULL;
    }
    if (iconList != NULL)
        SalamanderGUI->DestroyIconList(iconList);
}

void WINAPI CLocalLlamaPluginInterface::Event(int event, DWORD param)
{
    UNREFERENCED_PARAMETER(event);
    UNREFERENCED_PARAMETER(param);
    EnsureProvider();
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(reserved);
    if (reason == DLL_PROCESS_ATTACH) {
        DLLInstance = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

int WINAPI SalamanderPluginGetReqVer() { return LAST_VERSION_OF_SALAMANDER; }

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    SalamanderDebug = salamander->GetSalamanderDebug();
    SalamanderVersion = salamander->GetVersion();
    SalamanderGeneral = salamander->GetSalamanderGeneral();
    SalamanderGUI = salamander->GetSalamanderGUI();
    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
        return NULL;
    salamander->SetBasicPluginData(
        "SalamatrixAI Local Llama",
        FUNCTION_AUTOMATIONFRAMEWORK,
        VERSINFO_VERSION_NO_PLATFORM,
        VERSINFO_COPYRIGHT,
        VERSINFO_DESCRIPTION,
        VERSINFO_INTERNAL,
        NULL,
        NULL);
    salamander->SetPluginHomePageURL("https://samandarin.krtkovo.eu/");
    LocalLlamaPluginInterface.Event(0, 0);
    return &LocalLlamaPluginInterface;
}
