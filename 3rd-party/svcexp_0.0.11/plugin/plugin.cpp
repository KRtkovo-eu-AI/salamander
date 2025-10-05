#include "pch.h"


CPluginInterface PluginInterface;

// dalsi casti interfacu CPluginInterface
CPluginInterfaceForArchiver InterfaceForArchiver;
CPluginInterfaceForViewer InterfaceForViewer;
CPluginInterfaceForMenuExt InterfaceForMenuExt;
CPluginInterfaceForFS InterfaceForFS;
CPluginInterfaceForThumbLoader InterfaceForThumbLoader;

HINSTANCE DLLInstance = NULL;       // handle @ SPL
HINSTANCE HLanguage = NULL;         // handle @ SLG


CSalamanderGeneralAbstract		*SalamanderGeneral = NULL;
CSalamanderDebugAbstract		*SalamanderDebug = NULL;
CSalamanderGUIAbstract			*SalamanderGUI = NULL;


class C__StrCriticalSection
{
  public:
    CRITICAL_SECTION cs;

    C__StrCriticalSection() {HANDLES(InitializeCriticalSection(&cs));}
    ~C__StrCriticalSection() {HANDLES(DeleteCriticalSection(&cs));}
};

C__StrCriticalSection __StrCriticalSection;
char *LoadStr(int resID)
{
  static char buffer[5000];   // buffer pro mnoho stringu
  static char *act = buffer;

  HANDLES(EnterCriticalSection(&__StrCriticalSection.cs));

  if (5000 - (act - buffer) < 200) act = buffer;

#ifdef _DEBUG
  // radeji si pojistime, aby nas nekdo nevolal pred inicializaci handlu s resourcy
  if (HLanguage == NULL)
    TRACE_E("LoadStr: HLanguage == NULL");
#endif //_DEBUG

RELOAD:
  int size = LoadString(HLanguage, resID, act, 5000 - (act - buffer));
  // size obsahuje pocet nakopirovanych znaku bez terminatoru
//  DWORD error = GetLastError();
  char *ret;
  if (size != 0/* || error == NO_ERROR*/)     // error je NO_ERROR, i kdyz string neexistuje - nepouzitelne
  {
    if ((5000 - (act - buffer) == size + 1) && (act > buffer))
    {
      // pokud byl retezec presne na konci bufferu, mohlo
      // jit o oriznuti retezce -- pokud muzeme posunout okno
      // na zacatek bufferu, nacteme string jeste jednou
      act = buffer;
      goto RELOAD;
    }
    else
    {
      ret = act;
      act += size + 1;
    }
  }
  else
  {
    TRACE_E("Error in LoadStr(" << resID << ")." /*"): " << GetErrorText(error)*/);
    ret = "ERROR LOADING STRING";
  }

  HANDLES(LeaveCriticalSection(&__StrCriticalSection.cs));

  return ret;
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
	OutputDebugString ("SalamanderPluginEntry");
	if (salamander->GetVersion() < LAST_VERSION_OF_SALAMANDER)
	{  // starsi verze odmitneme
		MessageBox(salamander->GetParentWindow(),REQUIRE_LAST_VERSION_OF_SALAMANDER,
							 VERSINFO_PLUGINNAME, MB_OK | MB_ICONERROR);
		return NULL;
	}
	// give a general interface Salamander
	SalamanderDebug		= salamander->GetSalamanderDebug();
	SalamanderGeneral = salamander->GetSalamanderGeneral();
	SalamanderGUI			= salamander->GetSalamanderGUI();

	WORD LID = salamander->GetCurrentSalamanderLanguageID();

	// Load Language Module
	HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), "ServiceExplorer");
		if (HLanguage == NULL) return NULL;

  // configure the basic information about the plugin
	salamander->SetBasicPluginData2(VERSINFO_PLUGINNAME, FUNCTION_FILESYSTEM,
																	VERSINFO_VERSION, VERSINFO_COPYRIGHT, 
																	VERSINFO_DESCRIPTION,
																	"ServiceExplorer", "0", "svc" );
	// set home-page URL plugin
	salamander->SetPluginHomePageURL("http://www.jamik.de");

	SalamanderGeneral->GetPluginFSName(AssignedFSName, 0);

	if (!InitFS())
  {
		MessageBox( salamander->GetParentWindow(),REQUIRE_LAST_VERSION_OF_SALAMANDER,
								"Service Explorer", MB_OK | MB_ICONERROR);
    return NULL;  // chyba
  }

	return &PluginInterface;
}
int WINAPI SalamanderPluginGetReqVer()
{
	return PluginInterface.GetVersion();
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
	
	//IDB_FILEMGMT
	HBITMAP hBmp = (HBITMAP)HANDLES(LoadImage(DLLInstance, MAKEINTRESOURCE(IDB_SVC), IMAGE_BITMAP, 16, 16, LR_DEFAULTCOLOR));
	salamander->SetBitmapWithIcons(hBmp);
	HANDLES(DeleteObject(hBmp));

	salamander->SetPluginIcon(0);
	salamander->SetPluginMenuAndToolbarIcon(0);


	  if (!InitializeWinLib(VERSINFO_PLUGINNAME, DLLInstance)) 
		{
			//return FALSE;
		}
  SetWinLibStrings("Invalid number!", VERSINFO_PLUGINNAME);
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
  //_snprintf(buf, 1000, "Service Explorer 0.0.4 beta 2 \n\nCopyright © 2009 MJ\n\nBrowse and configure installed services");
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