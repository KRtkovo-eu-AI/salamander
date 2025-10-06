#include "precomp.h"
#include "eventviewerwindow.h"

CPluginInterface PluginInterface;
CPluginInterfaceForMenuExt InterfaceForMenuExt;

HINSTANCE DLLInstance = NULL;
HINSTANCE HLanguage = NULL;

CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderGUIAbstract* SalamanderGUI = NULL;
CSalamanderDebugAbstract* SalamanderDebug = NULL;

static std::unique_ptr<CEventViewerWindow> gEventViewerWindow;

int SalamanderVersion = 0;

HINSTANCE GetLanguageResourceHandle()
{
    return HLanguage != NULL ? HLanguage : DLLInstance;
}

char* LoadStr(int resID)
{
    if (SalamanderGeneral == NULL)
        return (char*)"";

    char* text = SalamanderGeneral->LoadStr(GetLanguageResourceHandle(), resID);
    return text != NULL ? text : (char*)"";
}

void EnsureEventViewerWindowClosed()
{
    if (gEventViewerWindow)
    {
        gEventViewerWindow->Close();
        gEventViewerWindow.reset();
    }
}

void ShowEventViewerWindow(HWND parent)
{
    if (!gEventViewerWindow)
    {
        gEventViewerWindow = std::make_unique<CEventViewerWindow>();
    }

    if (!gEventViewerWindow->IsCreated())
    {
        if (!gEventViewerWindow->Create(parent))
        {
            SalamanderGeneral->ShowMessageBox(LoadStr(IDS_EVENT_VIEWER_CREATE_FAILED), LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
            gEventViewerWindow.reset();
            return;
        }
    }

    gEventViewerWindow->Show();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DLLInstance = hinstDLL;

        INITCOMMONCONTROLSEX initCtrls;
        initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
        initCtrls.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES;
        if (!InitCommonControlsEx(&initCtrls))
        {
            return FALSE;
        }
    }

    return TRUE;
}

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    SalamanderDebug = salamander->GetSalamanderDebug();
    SalamanderVersion = salamander->GetVersion();
    HANDLES_CAN_USE_TRACE();

    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
    {
        MessageBox(salamander->GetParentWindow(), REQUIRE_LAST_VERSION_OF_SALAMANDER, VERSINFO_PLUGINNAME,
                   MB_OK | MB_ICONERROR);
        return NULL;
    }

    SalamanderGeneral = salamander->GetSalamanderGeneral();
    SalamanderGUI = salamander->GetSalamanderGUI();

    HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), "EventViewer");
    if (HLanguage == NULL)
        return NULL;

    SalamanderGeneral->SetHelpFileName("eventviewer.chm");

    salamander->SetBasicPluginData(LoadStr(IDS_PLUGINNAME), 0, VERSINFO_VERSION_NO_PLATFORM, VERSINFO_COPYRIGHT,
                                   LoadStr(IDS_PLUGIN_DESCRIPTION), "EVENTVIEWER", NULL, NULL);

    salamander->SetPluginHomePageURL(LoadStr(IDS_PLUGIN_HOME));

    return &PluginInterface;
}

void WINAPI CPluginInterface::About(HWND parent)
{
    char buf[1024];
    _snprintf_s(buf, _TRUNCATE, "%s " VERSINFO_VERSION "\n\n" VERSINFO_COPYRIGHT "\n\n%s",
                LoadStr(IDS_PLUGINNAME), LoadStr(IDS_PLUGIN_DESCRIPTION));
    SalamanderGeneral->ShowMessageBox(buf, LoadStr(IDS_ABOUT), MSGBOX_INFO);
}

void WINAPI CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    salamander->AddMenuItem(-1, LoadStr(IDS_EVENT_VIEWER_MENU), SALHOTKEY('L', HOTKEYF_CONTROL | HOTKEYF_SHIFT),
                            MENUCMD_EVENT_VIEWER, FALSE, MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
}

void WINAPI CPluginInterface::Event(int event, DWORD param)
{
    UNREFERENCED_PARAMETER(event);
    UNREFERENCED_PARAMETER(param);
}

BOOL WINAPI CPluginInterface::Release(HWND parent, BOOL force)
{
    EnsureEventViewerWindowClosed();
    return TRUE;
}

void WINAPI CPluginInterface::LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{
    // no configuration yet
}

void WINAPI CPluginInterface::SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{
    // no configuration yet
}

void WINAPI CPluginInterface::Configuration(HWND parent)
{
    SalamanderGeneral->ShowMessageBox(LoadStr(IDS_NO_CONFIGURATION), LoadStr(IDS_PLUGINNAME), MSGBOX_INFO);
}

void WINAPI CPluginInterface::ClearHistory(HWND parent)
{
    // nothing to clear
}

CPluginInterfaceForMenuExtAbstract* WINAPI CPluginInterface::GetInterfaceForMenuExt()
{
    return &InterfaceForMenuExt;
}

DWORD WINAPI CPluginInterfaceForMenuExt::GetMenuItemState(int id, DWORD eventMask)
{
    UNREFERENCED_PARAMETER(eventMask);
    if (id == MENUCMD_EVENT_VIEWER)
        return 0;
    return 0;
}

BOOL WINAPI CPluginInterfaceForMenuExt::ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander, HWND parent,
                                                        int id, DWORD eventMask)
{
    if (id == MENUCMD_EVENT_VIEWER)
    {
        ShowEventViewerWindow(parent);
        return FALSE;
    }
    return FALSE;
}

BOOL WINAPI CPluginInterfaceForMenuExt::HelpForMenuItem(HWND parent, int id)
{
    return FALSE;
}

void WINAPI CPluginInterfaceForMenuExt::BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* salamander)
{
    // menu structure handled by Salamander
}
