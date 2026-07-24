// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework plugin for Open Salamander

    This plugin is the first concrete provider of the Salamatrix service set. It
    owns the native RuntimeServices aggregate and registers versioned services in
    Salamander's process-local service registry so DemoPlug, future native
    plugins, and Automation runtimes can query them instead of creating their own
    ad-hoc runtime instances.
*/

#include "precomp.h"

CPluginInterface PluginInterface;

const char* PluginNameEN = "Salamatrix Framework";
const char* PluginNameShort = "SALAMATRIX";

HINSTANCE DLLInstance = NULL;
HINSTANCE HLanguage = NULL;
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderGUIAbstract* SalamanderGUI = NULL;
CSalamanderDebugAbstract* SalamanderDebug = NULL;
int SalamanderVersion = 0;

static Salamatrix::Runtime::RuntimeServices* SalamatrixRuntime = NULL;

static void DestroyRuntimeServices()
{
    delete SalamatrixRuntime;
    SalamatrixRuntime = NULL;
}

static BOOL CreateRuntimeServices()
{
    DestroyRuntimeServices();
    SalamatrixRuntime = new Salamatrix::Runtime::RuntimeServices(SalamanderGeneral, TRUE);
    if (SalamatrixRuntime == NULL || !SalamatrixRuntime->IsRegistered() || !SalamatrixRuntime->IsHostRegistered())
    {
        DestroyRuntimeServices();
        return FALSE;
    }
    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DLLInstance = hinstDLL;

        INITCOMMONCONTROLSEX initCtrls;
        initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
        initCtrls.dwICC = ICC_BAR_CLASSES | ICC_PROGRESS_CLASS | ICC_LISTVIEW_CLASSES;
        if (!InitCommonControlsEx(&initCtrls))
        {
            MessageBox(NULL, "InitCommonControlsEx failed!", PluginNameEN, MB_OK | MB_ICONERROR);
            return FALSE;
        }
    }
    return TRUE;
}

#ifdef __BORLANDC__
extern "C"
{
    int WINAPI SalamanderPluginGetReqVer();
    CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander);
};
#endif

int WINAPI SalamanderPluginGetReqVer()
{
    return LAST_VERSION_OF_SALAMANDER;
}

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    SalamanderDebug = salamander->GetSalamanderDebug();
    SalamanderVersion = salamander->GetVersion();
    HANDLES_CAN_USE_TRACE();
    CALL_STACK_MESSAGE1("SalamanderPluginEntry() - Salamatrix Framework");

    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
    {
        MessageBox(salamander->GetParentWindow(), REQUIRE_LAST_VERSION_OF_SALAMANDER, PluginNameEN, MB_OK | MB_ICONERROR);
        return NULL;
    }

    SalamanderGeneral = salamander->GetSalamanderGeneral();
    SalamanderGUI = salamander->GetSalamanderGUI();
    salamander->SetBasicPluginData(PluginNameEN, FUNCTION_AUTOMATIONFRAMEWORK, VERSINFO_VERSION_NO_PLATFORM, VERSINFO_COPYRIGHT,
                                   VERSINFO_DESCRIPTION,
                                   PluginNameShort, NULL, NULL);
    salamander->SetPluginHomePageURL("https://samandarin.krtkovo.eu/");

    SalamanderGeneral->SetFlagLoadOnSalamanderStart(TRUE);

    if (!CreateRuntimeServices())
    {
        SalamanderGeneral->SalMessageBox(salamander->GetParentWindow(),
                                         "Salamatrix Framework could not register its services. Another provider may already be active.",
                                         PluginNameEN, MB_OK | MB_ICONERROR);
        return NULL;
    }

    SalamatrixRuntime->Events()->PublishLifecycle(
        Salamatrix::Events::EventKindHostStartup);

    return &PluginInterface;
}

void WINAPI CPluginInterface::About(HWND parent)
{
    char buf[1000];
    _snprintf_s(buf, _TRUNCATE,
                "Salamatrix Framework 0.1\n\n"
                "Registered services: %s\n"
                "Service count: %d\n"
                "Runtime adapters: %d\n\n"
                "Provides Salamatrix.UI, Salamatrix.Commands, Salamatrix.FileOperations, Salamatrix.Runtime, Salamatrix.Sides, Salamatrix.Events, Salamatrix.Extensions, Salamatrix.Storage and the Automation adapter.",
                SalamatrixRuntime != NULL && SalamatrixRuntime->IsHostRegistered() ? "yes" : "no",
                SalamatrixRuntime != NULL ? SalamatrixRuntime->Services()->GetCount() : 0,
                SalamatrixRuntime != NULL ? SalamatrixRuntime->Runtimes()->GetAdapterCount() : 0);
    SalamanderGeneral->SalMessageBox(parent, buf, PluginNameEN, MB_OK | MB_ICONINFORMATION);
}

void WINAPI CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    CALL_STACK_MESSAGE1("CPluginInterface::Connect(,) - Salamatrix Framework");

    if (SalamanderGUI != NULL)
    {
        CGUIIconListAbstract* iconList = SalamanderGUI->CreateIconList();
        if (iconList != NULL)
        {
            if (iconList->Create(16, 16, 1))
            {
                UINT loadFlags = SalamanderGeneral != NULL ? SalamanderGeneral->GetIconLRFlags() : LR_DEFAULTCOLOR;
                HICON hIcon = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_PLUGINICON), IMAGE_ICON, 16, 16, loadFlags);
                if (hIcon != NULL)
                {
                    iconList->ReplaceIcon(0, hIcon);
                    DestroyIcon(hIcon);
                    salamander->SetIconListForGUI(iconList);
                    salamander->SetPluginIcon(0);
                    salamander->SetPluginMenuAndToolbarIcon(0);
                    iconList = NULL;
                }
            }

            if (iconList != NULL)
                SalamanderGUI->DestroyIconList(iconList);
        }
    }
}

void WINAPI CPluginInterface::LoadConfiguration(
    HWND parent,
    HKEY regKey,
    CSalamanderRegistryAbstract* registry)
{
    UNREFERENCED_PARAMETER(parent);
    if (SalamatrixRuntime != NULL)
        SalamatrixRuntime->Storage()->LoadConfiguration(regKey, registry);
}

void WINAPI CPluginInterface::SaveConfiguration(
    HWND parent,
    HKEY regKey,
    CSalamanderRegistryAbstract* registry)
{
    UNREFERENCED_PARAMETER(parent);
    if (SalamatrixRuntime != NULL)
        SalamatrixRuntime->Storage()->SaveConfiguration(regKey, registry);
}

void WINAPI CPluginInterface::Event(int event, DWORD param)
{
    if (SalamatrixRuntime != NULL)
        SalamatrixRuntime->Events()->PublishHostEvent(event, param);
}

BOOL WINAPI CPluginInterface::Release(HWND parent, BOOL force)
{
    if (SalamatrixRuntime != NULL)
        SalamatrixRuntime->Events()->PublishLifecycle(
            Salamatrix::Events::EventKindHostShutdown);
    DestroyRuntimeServices();
    SalamanderGeneral = NULL;
    SalamanderGUI = NULL;
    return TRUE;
}
