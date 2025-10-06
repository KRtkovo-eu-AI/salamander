#include "precomp.h"


CPluginInterface PluginInterface;

// dalsi casti interfacu CPluginInterface
CPluginInterfaceForArchiver InterfaceForArchiver;
CPluginInterfaceForViewer InterfaceForViewer;
CPluginInterfaceForMenuExt InterfaceForMenuExt;
CPluginInterfaceForFS InterfaceForFS;
CPluginInterfaceForThumbLoader InterfaceForThumbLoader;

HINSTANCE DLLInstance = NULL; // handle @ SPL
HINSTANCE HLanguage = NULL;   // handle @ SLG

namespace
{
HBITMAP CreateServiceBitmap()
{
  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biWidth = 16;
  bmi.bmiHeader.biHeight = -16;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  void *bits = NULL;
  HBITMAP bitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
  if (bitmap == NULL)
    return NULL;

  HDC dc = CreateCompatibleDC(NULL);
  if (dc == NULL)
  {
    DeleteObject(bitmap);
    return NULL;
  }

  HGDIOBJ old = SelectObject(dc, bitmap);
  HICON icon = reinterpret_cast<HICON>(LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_SERVICEEXPLORER_DIR),
                                                 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
  if (icon != NULL)
  {
    DrawIconEx(dc, 0, 0, icon, 16, 16, 0, NULL, DI_NORMAL);
    HANDLES(DestroyIcon(icon));
  }

  if (old != NULL)
    SelectObject(dc, old);
  DeleteDC(dc);

  return bitmap;
}
}

HINSTANCE GetLanguageResourceHandle()
{
  return HLanguage != NULL ? HLanguage : DLLInstance;
}

CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderDebugAbstract* SalamanderDebug = NULL;
CSalamanderGUIAbstract* SalamanderGUI = NULL;

int SalamanderVersion = 0;

char* LoadStr(int resID)
{
  if (SalamanderGeneral == NULL)
    return (char*)"";

  char* text = SalamanderGeneral->LoadStr(GetLanguageResourceHandle(), resID);
  return text != NULL ? text : (char*)"";
}

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
      //MessageBox(NULL, "DllMain-InitCommonControlsEx failed!", "Error", MB_OK | MB_ICONERROR);
      return FALSE;  // DLL won't start
    }
  }

  return TRUE;    // DLL can be loaded
}

CPluginInterfaceAbstract * WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract *salamander)
{
  SalamanderDebug = salamander->GetSalamanderDebug();
  SalamanderVersion = salamander->GetVersion();
  HANDLES_CAN_USE_TRACE();

  OutputDebugString("SalamanderPluginEntry");

  if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
  {
    MessageBox(salamander->GetParentWindow(), REQUIRE_LAST_VERSION_OF_SALAMANDER,
               VERSINFO_PLUGINNAME, MB_OK | MB_ICONERROR);
    return NULL;
  }

  SalamanderGeneral = salamander->GetSalamanderGeneral();
  SalamanderGUI = salamander->GetSalamanderGUI();

  HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), "ServiceExplorer");
  if (HLanguage == NULL && SalamanderDebug != NULL)
  {
    SalamanderDebug->TraceI(__FILE__, __LINE__,
                            "ServiceExplorer: missing language module, using built-in English resources.");
  }

  salamander->SetBasicPluginData(VERSINFO_PLUGINNAME, FUNCTION_FILESYSTEM,
                                 VERSINFO_VERSION_NO_PLATFORM, VERSINFO_COPYRIGHT,
                                 VERSINFO_DESCRIPTION, "ServiceExplorer", "0", "svc");
  salamander->SetPluginHomePageURL("http://www.jamik.de");

  SalamanderGeneral->GetPluginFSName(AssignedFSName, 0);

  if (!InitFS())
  {
    SalamanderGeneral->SalMessageBox(salamander->GetParentWindow(), REQUIRE_LAST_VERSION_OF_SALAMANDER,
                                     VERSINFO_PLUGINNAME, MB_OK | MB_ICONERROR);
    return NULL;
  }

  return &PluginInterface;
}

int WINAPI SalamanderPluginGetReqVer()
{
  return LAST_VERSION_OF_SALAMANDER;
}

void WINAPI CPluginInterface::About(HWND parent)
{
  OnAbout(parent);
}

BOOL WINAPI CPluginInterface::Release(HWND parent, BOOL force)
{
  return TRUE;
}
void WINAPI CPluginInterface::LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract *registry)
{}

void WINAPI CPluginInterface::SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract *registry)
{}

void WINAPI CPluginInterface::Configuration(HWND parent)
{
  OnConfiguration(parent);
}

void WINAPI CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract *salamander)
{
        salamander->SetChangeDriveMenuItem("\tWindows Services", 0);

        HBITMAP hBmp = CreateServiceBitmap();
        if (hBmp != NULL)
        {
          salamander->SetBitmapWithIcons(hBmp);
          HANDLES(DeleteObject(hBmp));
        }
        else
        {
          salamander->SetBitmapWithIcons(NULL);
        }

        salamander->SetPluginIcon(0);
        salamander->SetPluginMenuAndToolbarIcon(0);


        if (!InitializeWinLib(VERSINFO_PLUGINNAME, DLLInstance))
        {
                //return FALSE;
        }
        SetWinLibStrings("Invalid number!", VERSINFO_PLUGINNAME);
        EnsureTransferInfoStorage();
  //SetupWinLibHelp(HTMLHelpCallback);
  //return TRUE;

}
void WINAPI CPluginInterface::ReleasePluginDataInterface(CPluginDataInterfaceAbstract *pluginData)
{
	//if (pluginData != &ArcPluginDataInterface)  // Case allocate object FS data, loosen it
 // {
    delete ((CPluginFSDataInterface *)pluginData);
  //}
}

CPluginInterfaceForArchiverAbstract * WINAPI
CPluginInterface::GetInterfaceForArchiver()
{
  return &InterfaceForArchiver;
}

CPluginInterfaceForViewerAbstract * WINAPI
CPluginInterface::GetInterfaceForViewer()
{
  return &InterfaceForViewer;
}

CPluginInterfaceForMenuExtAbstract * WINAPI
CPluginInterface::GetInterfaceForMenuExt()
{
  return &InterfaceForMenuExt;
}

CPluginInterfaceForFSAbstract * WINAPI
CPluginInterface::GetInterfaceForFS()
{
  return &InterfaceForFS;
}

CPluginInterfaceForThumbLoaderAbstract * WINAPI
CPluginInterface::GetInterfaceForThumbLoader()
{
  return &InterfaceForThumbLoader;
}

void WINAPI
CPluginInterface::Event(int event, DWORD param)
{
}
void WINAPI
CPluginInterface::ClearHistory(HWND parent)
{ }

// -------------------------------------------------------------------------------------------------------
// Local Implementations
// -------------------------------------------------------------------------------------------------------
void OnAbout(HWND hParent)
{
  OutputDebugString ("OnAbout");

  char buf[1000];
  buf[999] = 0;
  //_snprintf(buf, 1000, "Service Explorer 0.0.4 beta 2 \n\nCopyright  2009 MJ\n\nBrowse and configure installed services");
	//SalamanderGeneral->SalMessageBox(hParent, buf, "About Plugin", MB_OK | MB_ICONINFORMATION);

	  _snprintf(buf, 1000, 
            "%s " VERSINFO_VERSION "\n\n"
            VERSINFO_COPYRIGHT "\n\n"
            "%s",
            VERSINFO_PLUGINNAME, 
            LoadStr(IDS_PLUGIN_DESCRIPTION));

  SalamanderGeneral->SalMessageBox(hParent, buf, LoadStr(IDS_ABOUT), MB_OK | MB_ICONINFORMATION);
}

void OnConfiguration(HWND hParent)
{

}