// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

//****************************************************************************
//
// Copyright (c) 2023 Open Salamander Authors
//
// This is a part of the Open Salamander SDK library.
//
//****************************************************************************

#include "precomp.h"

// plugin interface object; Salamander calls its methods
CPluginInterface PluginInterface;
// additional parts of the CPluginInterface interface
CPluginInterfaceForMenuExt InterfaceForMenuExt;

// global data
const char* PluginNameEN = "C# Demo";    // untranslated plugin name, used before the language module loads and for debugging
const char* PluginNameShort = "CSDEMO"; // plugin name (short, without spaces)

HINSTANCE DLLInstance = NULL; // handle of the SPL - language-neutral resources
HINSTANCE HLanguage = NULL;   // handle of the SLG - language-specific resources

// generic Salamander interface - valid from startup until the plugin shuts down
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;

// variable definition for "dbg.h"
CSalamanderDebugAbstract* SalamanderDebug = NULL;

// variable definition for "spl_com.h"
int SalamanderVersion = 0;

// interface providing custom Windows controls used in Salamander
//CSalamanderGUIAbstract *SalamanderGUI = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DLLInstance = hinstDLL;

        INITCOMMONCONTROLSEX initCtrls;
        initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
        initCtrls.dwICC = ICC_BAR_CLASSES;
        if (!InitCommonControlsEx(&initCtrls))
        {
            MessageBox(NULL, "InitCommonControlsEx failed!", "Error", MB_OK | MB_ICONERROR);
            return FALSE; // DLL won't start
        }
    }

    return TRUE; // DLL can be loaded
}

// ****************************************************************************

char* LoadStr(int resID)
{
    return SalamanderGeneral->LoadStr(HLanguage, resID);
}

void OnAbout(HWND hParent)
{
    if (!ManagedBridge_ShowAbout(hParent))
    {
        SalamanderGeneral->SalMessageBox(hParent,
                                         "Unable to open the managed About dialog.\n"
                                         "Verify that CSDemo.Managed.dll is located next to the plugin.",
                                         LoadStr(IDS_PLUGINNAME), MB_OK | MB_ICONERROR);
    }
}

//
// ****************************************************************************
// SalamanderPluginGetReqVer
//

#ifdef __BORLANDC__
extern "C"
{
    int WINAPI SalamanderPluginGetReqVer();
    CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander);
};
#endif // __BORLANDC__

int WINAPI SalamanderPluginGetReqVer()
{
    return LAST_VERSION_OF_SALAMANDER;
}

//
// ****************************************************************************
// SalamanderPluginEntry
//

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    // configure SalamanderDebug for "dbg.h"
    SalamanderDebug = salamander->GetSalamanderDebug();
    // configure SalamanderVersion for "spl_com.h"
    SalamanderVersion = salamander->GetVersion();
    HANDLES_CAN_USE_TRACE();
    CALL_STACK_MESSAGE1("SalamanderPluginEntry()");

    // the plugin targets the current version of Salamander and newer - verify the requirement
    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
    { // reject older versions
        MessageBox(salamander->GetParentWindow(),
                   REQUIRE_LAST_VERSION_OF_SALAMANDER,
                   PluginNameEN, MB_OK | MB_ICONERROR);
        return NULL;
    }

    // load the language module (.slg)
    HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), PluginNameEN);
    if (HLanguage == NULL)
        return NULL;

    // obtain the general Salamander interface
    SalamanderGeneral = salamander->GetSalamanderGeneral();
    // obtain the interface providing custom Windows controls used in Salamander
    //  SalamanderGUI = salamander->GetSalamanderGUI();

    // set the help file name
    SalamanderGeneral->SetHelpFileName("csdemo.chm");

    // set the basic plugin metadata
    salamander->SetBasicPluginData(LoadStr(IDS_PLUGINNAME), FUNCTION_DYNAMICMENUEXT | FUNCTION_CONFIGURATION,
                                   VERSINFO_VERSION_NO_PLATFORM, VERSINFO_COPYRIGHT,
                                   LoadStr(IDS_PLUGIN_DESCRIPTION), PluginNameShort,
                                   NULL, NULL);

    // set the plugin home page URL
    salamander->SetPluginHomePageURL(LoadStr(IDS_PLUGIN_HOME));

    return &PluginInterface;
}

//
// ****************************************************************************
// CPluginInterface
//

void WINAPI
CPluginInterface::About(HWND parent)
{
    OnAbout(parent);
}

BOOL WINAPI
CPluginInterface::Release(HWND parent, BOOL force)
{
    ManagedBridge_Shutdown();
    return TRUE;
}

void WINAPI
CPluginInterface::Configuration(HWND parent)
{
    if (!ManagedBridge_ShowConfiguration(parent))
    {
        SalamanderGeneral->SalMessageBox(parent,
                                         "Unable to open the managed configuration window.",
                                         LoadStr(IDS_PLUGINNAME), MB_OK | MB_ICONERROR);
    }
}

void WINAPI
CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    CALL_STACK_MESSAGE1("CPluginInterface::Connect(,)");

    // basic part:
    salamander->AddMenuItem(-1, LoadStr(IDS_MENU_HELLO), SALHOTKEY('M', HOTKEYF_CONTROL | HOTKEYF_SHIFT),
                            MENUCMD_SHOWHELLO, FALSE, MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);

    ManagedBridge_EnsureInitialized(parent);

    /*
  CGUIIconListAbstract *iconList = SalamanderGUI->CreateIconList();
  iconList->Create(16, 16, 1);
  HICON hIcon = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_PLUGINICON), IMAGE_ICON, 16, 16, SalamanderGeneral->GetIconLRFlags());
  iconList->ReplaceIcon(0, hIcon);
  DestroyIcon(hIcon);
  salamander->SetIconListForGUI(iconList); // Salamander takes care of destroying the icon list

  salamander->SetPluginIcon(0);
  salamander->SetPluginMenuAndToolbarIcon(0);
*/
}

CPluginInterfaceForMenuExtAbstract* WINAPI
CPluginInterface::GetInterfaceForMenuExt()
{
    return &InterfaceForMenuExt;
}
