#include "precomp.h"

namespace
{
char *DupStringOrEmpty(const char *text)
{
  const char *source = (text != NULL) ? text : "";
  char *dup = SalamanderGeneral->DupStr(source);
  if (dup == NULL && source[0] != '\0')
  {
    dup = SalamanderGeneral->DupStr("");
  }
  return dup;
}

void PopulateServiceConfiguration(const char *serviceName, int &startupType, char *&logOnAs, char *&executablePath)
{
  startupType = 99;
  logOnAs = NULL;
  executablePath = NULL;

  SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
  if (scm == NULL)
  {
    return;
  }

  SC_HANDLE service = OpenService(scm, serviceName, SERVICE_QUERY_CONFIG);
  if (service == NULL)
  {
    CloseServiceHandle(scm);
    return;
  }

  DWORD bytesNeeded = 0;
  if (!QueryServiceConfig(service, NULL, 0, &bytesNeeded) && GetLastError() == ERROR_INSUFFICIENT_BUFFER && bytesNeeded != 0)
  {
    std::vector<BYTE> buffer(bytesNeeded);
    QUERY_SERVICE_CONFIG *config = reinterpret_cast<QUERY_SERVICE_CONFIG *>(buffer.data());
    if (QueryServiceConfig(service, config, bytesNeeded, &bytesNeeded))
    {
      startupType = static_cast<int>(config->dwStartType);

      logOnAs = DupStringOrEmpty(config->lpServiceStartName);
      executablePath = DupStringOrEmpty(config->lpBinaryPathName);
    }
  }

  CloseServiceHandle(service);
  CloseServiceHandle(scm);
}
} // namespace

// ****************************************************************************
// CPluginFSInterface
// ****************************************************************************

// [1]
CPluginFSInterface::CPluginFSInterface()
{
  Path[0] = 0;
  PathError = FALSE;
  FatalError = FALSE;
  CalledFromDisconnectDialog = FALSE;
	
	OutputDebugString("fs2-CPluginFSInterface"); 
}

void WINAPI CPluginFSInterface::ReleaseObject(HWND parent)
{
	OutputDebugString("fs2-ReleaseObject"); 
}

BOOL WINAPI CPluginFSInterface::GetRootPath(char *userPart)
{
	OutputDebugString("fs2-GetRootPath"); 
	//TODO: Multiple Computers!?
	strcpy(userPart, "\\");
	//userPart[0]=0;
  return TRUE;
}

BOOL WINAPI CPluginFSInterface::GetCurrentPath(char *userPart)
{
	OutputDebugString("fs2-GetCurrentPath");
	strcpy(userPart, "\\");
  return TRUE;
}

BOOL WINAPI CPluginFSInterface::GetFullName(CFileData &file, int isDir, char *buf, int bufSize)
{
	OutputDebugString("fs2-GetFullName");
	return TRUE;
}

BOOL WINAPI CPluginFSInterface::GetFullFSPath(HWND parent, const char *fsName, char *path, int pathSize, BOOL &success)
{
	OutputDebugString("fs2-GetFullFSPath");
  return TRUE;
}

BOOL WINAPI CPluginFSInterface::IsCurrentPath(int currentFSNameIndex, int fsNameIndex, const char *userPart)
{
	OutputDebugString("fs2-IsCurrentPath");
  return TRUE;
}

BOOL WINAPI CPluginFSInterface::IsOurPath(int currentFSNameIndex, int fsNameIndex, const char *userPart)
{   
	OutputDebugString("fs2-IsOurPath");
	return TRUE;
}

BOOL WINAPI CPluginFSInterface::ChangePath(int currentFSNameIndex, char *fsName, int fsNameIndex,
                               const char *userPart, char *cutFileName, BOOL *pathWasCut,
                               BOOL forceRefresh, int mode)
{

	OutputDebugString("fs2-ChangePath"); 
  PathError = FALSE;
	return TRUE;
}

BOOL WINAPI CPluginFSInterface::ListCurrentPath(CSalamanderDirectoryAbstract *dir,
                                    CPluginDataInterfaceAbstract *&pluginData,
                                    int &iconsType, BOOL forceRefresh)
{
	OutputDebugString("fs2-ListCurrentPath"); 
	dir->SetValidData(VALID_DATA_NONE);
	CFileData file;
	pluginData = new CPluginFSDataInterface(Path);
	char *name; 
	DWORD err; 
	PathError = false;
	
	iconsType = pitFromPlugin;

	//Create Servicelist
	SC_HANDLE sc = ::OpenSCManager (NULL,NULL,SC_MANAGER_ENUMERATE_SERVICE);
	if (sc != NULL)
	{
		//Successfully opened SCM
		ENUM_SERVICE_STATUS service_data;
		ENUM_SERVICE_STATUS *lpservice = &service_data;
		BOOL retVal;
		DWORD bytesNeeded,resumeHandle = 0,srvType, srvState;
		DWORD srvCount=0;

		srvType = SERVICE_WIN32;
		srvState = SERVICE_STATE_ALL;

		//Call EnumServicesStatus using the handle returned by OpenSCManager
		retVal = ::EnumServicesStatus (sc,srvType,srvState,&service_data,sizeof(service_data), &bytesNeeded,&srvCount,&resumeHandle);

		err = GetLastError();

		//Check if EnumServicesStatus needs more memory space
		if ((retVal == FALSE) || err == ERROR_MORE_DATA)
		{
			 DWORD dwBytes = bytesNeeded + sizeof(ENUM_SERVICE_STATUS);
			 lpservice = new ENUM_SERVICE_STATUS [dwBytes];
			 EnumServicesStatus (sc,srvType,srvState,lpservice,dwBytes,
                                                        &bytesNeeded,&srvCount,&resumeHandle);
		}
		for(int i=0;i<(int)srvCount;i++)
		{
				name = lpservice[i].lpDisplayName;
				//lpservice[i].ServiceStatus

				file.Name = SalamanderGeneral->DupStr(name);
				file.NameLen = strlen(file.Name);
				file.Ext = file.Name + file.NameLen;
				//file.Size = CQuadWord(data.nFileSizeLow, data.nFileSizeHigh);
				//file.Attr = data.dwFileAttributes;
				//file.LastWrite = data.ftLastWriteTime;
				//file.Hidden = file.Attr & FILE_ATTRIBUTE_HIDDEN ? 1 : 0;
				file.DosName = NULL;
				file.IsLink = 0;
				file.IsOffline = 0;
				file.Hidden =0;
				file.Attr = 0;
                                DWORD ServiceStatus = GetServiceStatus(lpservice[i].lpServiceName);
                                int ServiceStartupType = 99;
                                char *LogOnAs = NULL;
                                char *ExecuteablePath = NULL;
                                PopulateServiceConfiguration(lpservice[i].lpServiceName, ServiceStartupType, LogOnAs, ExecuteablePath);
                                if (LogOnAs == NULL)
                                {
                                        LogOnAs = DupStringOrEmpty("");
                                }
                                if (ExecuteablePath == NULL)
                                {
                                        ExecuteablePath = DupStringOrEmpty("");
                                }
				//DoQuerySvc(lpservice[i].lpServiceName);

                               CFSData *extData;
                               extData = new CFSData();
                                extData->Description = DupStringOrEmpty("WeDon'tUseThisCurrently");
                                extData->Status = static_cast<int>(ServiceStatus);
                                extData->StartupType = ServiceStartupType;
                                extData->ServiceName = DupStringOrEmpty(lpservice[i].lpServiceName);
                                extData->LogOnAs = LogOnAs;
                                extData->ExecuteablePath = ExecuteablePath;
                                extData->DisplayName = DupStringOrEmpty(file.Name);
				


				file.PluginData = reinterpret_cast<DWORD_PTR>(extData);		//extData holds the information about the service

				// ADD the Item
				dir->AddFile(NULL, file, pluginData);

    }
		if (lpservice != &service_data)
		{
			delete [] lpservice;
		}
        }
	CloseServiceHandle(sc);
  return TRUE;
}

BOOL CALLBACK ConnectDlgProc(HWND HWindow, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  CALL_STACK_MESSAGE4("ConnectDlgProc(, 0x%X, %p, %p)",
                      uMsg,
                      reinterpret_cast<void*>(static_cast<UINT_PTR>(wParam)),
                      reinterpret_cast<void*>(static_cast<UINT_PTR>(lParam)));
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      // Horizontally and vertically centered dialogue to parent
      HWND hParent = GetParent(HWindow);
      if (hParent != NULL)
        SalamanderGeneral->MultiMonCenterWindow(HWindow, hParent, TRUE);

      SetFocus(GetDlgItem(HWindow, IDC_OK));  // chceme svuj vlastni fokus
      
      return TRUE;  // focus from std. dialogproc
    }
		case WM_PAINT:
		{
      // Horizontally and vertically centered dialogue to parent
      HWND hParent = GetParent(HWindow);
      if (hParent != NULL)
        SalamanderGeneral->MultiMonCenterWindow(HWindow, hParent, TRUE);

      SetFocus(GetDlgItem(HWindow, IDC_OK));  // chceme svuj vlastni fokus
      
      return TRUE;  // focus from std. dialogproc
    }


    case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
        case IDOK:
        {
          // obtaining data from the dialogue
          //SendDlgItemMessage(HWindow, IDC_PATH, WM_GETTEXT, MAX_PATH, (LPARAM)ConnectPath);
          //SalamanderGeneral->AddValueToStdHistoryValues(History, HistoryCount, ConnectPath, FALSE);
        }
        case IDCANCEL:
        {
          EndDialog(HWindow, wParam);
          return TRUE;
        }
      }
      break;
    }
  }
  return TRUE;    // not processed
  return FALSE;    // not processed
}

BOOL WINAPI CPluginFSInterface::TryCloseOrDetach(BOOL forceClose, BOOL canDetach, BOOL &detach, int reason)
{
	OutputDebugString("fs2-TryCloseOrDetach"); 
	return TRUE;
}

void WINAPI CPluginFSInterface::Event(int event, DWORD param)
{
	OutputDebugString("fs2-Event"); 
}

DWORD WINAPI CPluginFSInterface::GetSupportedServices()
{
  return FS_SERVICE_CONTEXTMENU |
				 FS_SERVICE_SHOWPROPERTIES |
         FS_SERVICE_ACCEPTSCHANGENOTIF |
         FS_SERVICE_SHOWINFO |
         FS_SERVICE_GETFSICON |
				 FS_SERVICE_DELETE |
         FS_SERVICE_GETPATHFORMAINWNDTITLE;
}

BOOL WINAPI CPluginFSInterface::GetChangeDriveOrDisconnectItem(const char *fsName, char *&title, HICON &icon, BOOL &destroyIcon)
{
	OutputDebugString("fs2-GetChangeDriveOrDisconnectItem"); 
  return TRUE;
}

HICON WINAPI CPluginFSInterface::GetFSIcon(BOOL &destroyIcon)
{
  char root[MAX_PATH];
  SalamanderGeneral->GetRootPath(root, Path);

  HICON icon;
  if (!SalamanderGeneral->GetFileIcon(root, FALSE, &icon, SALICONSIZE_16, TRUE, TRUE))
    icon = NULL;

  destroyIcon = TRUE;
  return icon;
	//return NULL;
}

void WINAPI CPluginFSInterface::GetDropEffect(const char *srcFSPath, const char *tgtFSPath,
                                  DWORD allowedEffects, DWORD keyState, DWORD *dropEffect)
{
	OutputDebugString("fs2-GetDropEffect"); 
}

void WINAPI CPluginFSInterface::GetFSFreeSpace(CQuadWord *retValue)
{
	OutputDebugString("fs2-GetFSFreeSpace"); 
}

BOOL WINAPI CPluginFSInterface::GetNextDirectoryLineHotPath(const char *text, int pathLen, int &offset)
{
	OutputDebugString("fs2-GetNextDirectoryLineHotPath"); 
	return TRUE;
}

void WINAPI CPluginFSInterface::ShowInfoDialog(const char *fsName, HWND parent)
{
	//TODO: Property Dialog
	OutputDebugString("fs2-ShowInfoDialog"); 
}

BOOL WINAPI CPluginFSInterface::ExecuteCommandLine(HWND parent, char *command, int &selFrom, int &selTo)
{
	return TRUE;
}

BOOL WINAPI CPluginFSInterface::QuickRename(const char *fsName, int mode, HWND parent, CFileData &file, BOOL isDir,
                                char *newName, BOOL &cancel)
{
	return TRUE;
}

void WINAPI CPluginFSInterface::AcceptChangeOnPathNotification(const char *fsName, const char *path, BOOL includingSubdirs)
{
	SalamanderGeneral->PostRefreshPanelFS(this);
}

BOOL WINAPI CPluginFSInterface::CreateDir(const char *fsName, int mode, HWND parent, char *newName, BOOL &cancel)
{
    return TRUE;
}

void DeleteSvc(char* szSvName)
{

}

void WINAPI CPluginFSInterface::ViewFile(const char *fsName, HWND parent,
                             CSalamanderForViewFileOnFSAbstract *salamander,
                             CFileData &file)
{
}

BOOL WINAPI CPluginFSInterface::Delete(const char *fsName, int mode, HWND parent, int panel,
                           int selectedFiles, int selectedDirs, BOOL &cancelOrError)
{

	BOOL isDir = FALSE;  // TRUE if the 'f' is a directory
	BOOL focused = (selectedFiles == 0 && selectedDirs == 0);
	int index = 0;

	const CFileData *f = NULL; // pointer to the file / directory in the panel to be processed
	const CFileData **ItemFileData = NULL;
	CFSData *FSIdata;


	char buf[100];
	char bufcaption[100];

                                DWORD returnstate=0;
                                DWORD messagereturn=0;
                                bool refreshPanel = false;
	while (1)
	{
			if (focused) f = SalamanderGeneral->GetPanelFocusedItem(panel, &isDir);
			else f = SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
			
			if (f!= NULL)
			{
				FSIdata = (CFSData*)(f->PluginData);
				_snprintf(buf, 100, LoadStr(IDS_SERVICE_DELETE_CONFIRMATION),FSIdata->DisplayName);
				_snprintf(bufcaption, 100, LoadStr(IDS_IDS_SERVICE_DELETE_DLG_CAPTION),FSIdata->DisplayName);

				messagereturn = SalamanderGeneral->SalMessageBox(parent, buf, bufcaption, MB_YESNOCANCEL | MB_ICONQUESTION);
				switch (messagereturn)
				{
					case IDYES:
						returnstate=DoDeleteSvc(FSIdata->ServiceName);
						SalamanderGeneral->ToLowerCase(FSIdata->ServiceName);
						SalamanderGeneral->RemoveOneFileFromCache(FSIdata->ServiceName);
						SalamanderGeneral->PostChangeOnPathNotification("svc:\\", false);
						break;
					case IDNO:
						//SalamanderGeneral->ToLowerCase(FSIdata->ServiceName);
						//SalamanderGeneral->RemoveOneFileFromCache(FSIdata->ServiceName);
						break;
					case IDCANCEL:
						cancelOrError = true;
						SalamanderGeneral->PostChangeOnPathNotification("svc:\\", false);
						return TRUE;
						break;
				}
				if (returnstate>0)
				{
					char errormessage[100];
					switch (returnstate)
					{
						case 5:
							strcpy(errormessage,LoadStr(IDS_SERVICE_ERROR_INSUFFICIENTRIGHTS));
							break;
						case 1072:
							strcpy(errormessage,LoadStr(IDS_SERVICE_ERROR_MARKEDFORDELETION));
							break;
						default:
							strcpy(errormessage,LoadStr(IDS_SERVICE_ERROR_UNKNOWN));
					}
					char buf[500];
					buf[499] = 0;
					_snprintf(buf, 500, 
							"%s\n\n" 
							"%s %d: %s",LoadStr(IDS_SERVICE_ERROR_OPERATION),LoadStr(IDS_SEVICE_ERROR_CODE) ,returnstate,errormessage);
					SalamanderGeneral->SalMessageBox(parent, buf, VERSINFO_PLUGINNAME, MB_OK | MB_ICONWARNING);
					returnstate = 0;
				}
			}
			else
			{
				break;
			}
	}
	SalamanderGeneral->PostChangeOnPathNotification("svc:\\", false);
	cancelOrError = false;
	return TRUE;	
}

enum CDFSPathError
{
  dfspeNone,
  dfspeServerNameMissing,
  dfspeShareNameMissing,
  dfspeRelativePath,      // relativni cesty nejsou podporovany ("PATH", "\PATH", ani "C:PATH")
};



BOOL WINAPI CPluginFSInterface::CopyOrMoveFromFS(BOOL copy, int mode, const char *fsName, HWND parent,
                                     int panel, int selectedFiles, int selectedDirs,
                                     char *targetPath, BOOL &operationMask,
                                     BOOL &cancelOrHandlePath, HWND dropTarget)
{
  return TRUE;
}
BOOL WINAPI CPluginFSInterface::CopyOrMoveFromDiskToFS(BOOL copy, int mode, const char *fsName, HWND parent,
                                           const char *sourcePath, SalEnumSelection2 next,
                                           void *nextParam, int sourceFiles, int sourceDirs,
                                           char *targetPath, BOOL *invalidPathOrCancel)
{
	return TRUE;
}


BOOL WINAPI CPluginFSInterface::ChangeAttributes(const char *fsName, HWND parent, int panel,
                                     int selectedFiles, int selectedDirs)
{
  return TRUE;
}

void WINAPI CPluginFSInterface::ShowProperties(const char *fsName, HWND parent, int panel,
                                   int selectedFiles, int selectedDirs)
{
}

void WINAPI CPluginFSInterface::ContextMenu(const char *fsName, HWND parent, int menuX, int menuY, int type,
                                int panel, int selectedFiles, int selectedDirs)
{
        OutputDebugString("fs2-ContextMenu");

        const CFileData *f = NULL; // pointer to the file / directory in the panel to be processed
        BOOL isDir = FALSE;  // TRUE if the 'f' is a directory
        int index = 0;

        if (type == fscmItemsInPanel)
        {
                if (selectedFiles == 0 && selectedDirs == 0)
                        f = SalamanderGeneral->GetPanelFocusedItem(panel, &isDir);
                else
                        f = SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
        }

        CFSData *FSIdata = (f != NULL) ? (CFSData *)(f->PluginData) : NULL;    //Plugin Item Data
        DWORD returnstate = 0;

        if (type != fscmItemsInPanel || FSIdata == NULL)
        {
                HMENU menu = CreatePopupMenu();
                if (menu == NULL)
                        return;

                MENUITEMINFO mi;
                char nameBuf[200];
                int insertIndex = 0;

                strcpy(nameBuf, "Register &New Service...");
                memset(&mi, 0, sizeof(mi));
                mi.cbSize = sizeof(mi);
                mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
                mi.fType = MFT_STRING;
                mi.wID = MENUCMD_REGISTER;
                mi.dwTypeData = nameBuf;
                mi.cch = static_cast<UINT>(strlen(nameBuf));
                mi.fState = MFS_ENABLED;
                InsertMenuItem(menu, insertIndex++, TRUE, &mi);

                strcpy(nameBuf, "&Launch Service Control Manager");
                memset(&mi, 0, sizeof(mi));
                mi.cbSize = sizeof(mi);
                mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
                mi.fType = MFT_STRING;
                mi.wID = MENUCMD_SCM;
                mi.dwTypeData = nameBuf;
                mi.cch = static_cast<UINT>(strlen(nameBuf));
                mi.fState = MFS_ENABLED;
                InsertMenuItem(menu, insertIndex++, TRUE, &mi);

                DWORD cmd = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                                             menuX, menuY, parent, NULL);

                if (cmd == MENUCMD_REGISTER)
                {
                        if (RegisterNewService(parent))
                                SalamanderGeneral->PostRefreshPanelFS(this);
                }
                else if (cmd == MENUCMD_SCM)
                {
                        ShellExecute(0, "open", "services.msc", "", "", SW_SHOW);
                }

                DestroyMenu(menu);
                SalamanderGeneral->PostChangeOnPathNotification("svc\\", false);
                return;
        }

        int CurrentState = FSIdata->Status;
        int StartupType = FSIdata->StartupType;

        UINT START_State                = MFS_DISABLED;
        UINT STOP_State                 = MFS_DISABLED;
        UINT PAUSE_State                = MFS_DISABLED;
        UINT RESUME_State               = MFS_DISABLED;
        UINT RESTART_State              = MFS_DISABLED;

        if (StartupType != SERVICE_DISABLED)
        {
                switch (CurrentState)
                {
                        case SERVICE_STOPPED:
                                START_State = MFS_ENABLED;
                                break;
                        case SERVICE_RUNNING:
                                STOP_State = MFS_ENABLED;
                                RESTART_State = MFS_ENABLED;
                                break;
                        default:
                                break;
                }
        }
        else if (CurrentState == SERVICE_RUNNING)
        {
                STOP_State = MFS_ENABLED;
        }

        HMENU menu = CreatePopupMenu();
        if (menu == NULL)
                return;

        MENUITEMINFO mi;
        char nameBuf[200];

        switch (type)
        {
        case fscmItemsInPanel:
                {
                        int i = 0;

                        strcpy(nameBuf, "&Start");
                        memset(&mi, 0, sizeof(mi));
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
                        mi.fType = MFT_STRING;
                        mi.wID = MENUCMD_START;
                        mi.dwTypeData = nameBuf;
                        mi.cch = static_cast<UINT>(strlen(nameBuf));
                        mi.fState = START_State;
                        InsertMenuItem(menu, i++, TRUE, &mi);

                        strcpy(nameBuf, "S&top");
                        memset(&mi, 0, sizeof(mi));
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
                        mi.fType = MFT_STRING;
                        mi.wID = MENUCMD_STOP;
                        mi.dwTypeData = nameBuf;
                        mi.cch = static_cast<UINT>(strlen(nameBuf));
                        mi.fState = STOP_State;
                        InsertMenuItem(menu, i++, TRUE, &mi);

                        strcpy(nameBuf, "Register &New Service...");
                        memset(&mi, 0, sizeof(mi));
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
                        mi.fType = MFT_STRING;
                        mi.wID = MENUCMD_REGISTER;
                        mi.dwTypeData = nameBuf;
                        mi.cch = static_cast<UINT>(strlen(nameBuf));
                        mi.fState = MFS_ENABLED;
                        InsertMenuItem(menu, i++, TRUE, &mi);

                        memset(&mi, 0, sizeof(mi));
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE;
                        mi.fType = MFT_SEPARATOR;
                        InsertMenuItem(menu, i++, TRUE, &mi);

                        strcpy(nameBuf, "Pa&use");
                        memset(&mi, 0, sizeof(mi));
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
                        mi.fType = MFT_STRING;
                        mi.wID = MENUCMD_PAUSE;
                        mi.dwTypeData = nameBuf;
                        mi.cch = static_cast<UINT>(strlen(nameBuf));
                        mi.fState = PAUSE_State;
                        InsertMenuItem(menu, i++, TRUE, &mi);

                        strcpy(nameBuf, "Resu&me");
                        memset(&mi, 0, sizeof(mi));
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
                        mi.fType = MFT_STRING;
                        mi.wID = MENUCMD_RESUME;
                        mi.dwTypeData = nameBuf;
                        mi.cch = static_cast<UINT>(strlen(nameBuf));
                        mi.fState = RESUME_State;
                        InsertMenuItem(menu, i++, TRUE, &mi);

                        strcpy(nameBuf, "R&estart");
                        memset(&mi, 0, sizeof(mi));
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
                        mi.fType = MFT_STRING;
                        mi.wID = MENUCMD_RESTART;
                        mi.dwTypeData = nameBuf;
                        mi.cch = static_cast<UINT>(strlen(nameBuf));
                        mi.fState = RESTART_State;
                        InsertMenuItem(menu, i++, TRUE, &mi);

                        memset(&mi, 0, sizeof(mi));
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE;
                        mi.fType = MFT_SEPARATOR;
                        InsertMenuItem(menu, i++, TRUE, &mi);

                        const bool showDelete = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                        if (showDelete)
                        {
                                strcpy(nameBuf, "&Delete");
                                memset(&mi, 0, sizeof(mi));
                                mi.cbSize = sizeof(mi);
                                mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
                                mi.fType = MFT_STRING;
                                mi.wID = MENUCMD_DELETE;
                                mi.dwTypeData = nameBuf;
                                mi.cch = static_cast<UINT>(strlen(nameBuf));
                                mi.fState = MFS_ENABLED;
                                InsertMenuItem(menu, i++, TRUE, &mi);

                                memset(&mi, 0, sizeof(mi));
                                mi.cbSize = sizeof(mi);
                                mi.fMask = MIIM_TYPE;
                                mi.fType = MFT_SEPARATOR;
                                InsertMenuItem(menu, i++, TRUE, &mi);
                        }

                        strcpy(nameBuf, "P&roperties");
                        memset(&mi, 0, sizeof(mi));
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
                        mi.fType = MFT_STRING;
                        mi.wID = MENUCMD_PROPERTIES;
                        mi.dwTypeData = nameBuf;
                        mi.cch = static_cast<UINT>(strlen(nameBuf));
                        mi.fState = MFS_ENABLED | MFS_DEFAULT;
                        InsertMenuItem(menu, i++, TRUE, &mi);

                        memset(&mi, 0, sizeof(mi));
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE;
                        mi.fType = MFT_SEPARATOR;
                        InsertMenuItem(menu, i++, TRUE, &mi);

                        strcpy(nameBuf, "&Launch Service Control Manager");
                        memset(&mi, 0, sizeof(mi));
                        mi.cbSize = sizeof(mi);
                        mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
                        mi.fType = MFT_STRING;
                        mi.wID = MENUCMD_SCM;
                        mi.dwTypeData = nameBuf;
                        mi.cch = static_cast<UINT>(strlen(nameBuf));
                        mi.fState = MFS_ENABLED;
                        InsertMenuItem(menu, i++, TRUE, &mi);

                        CConfigDialog dlg(parent, FSIdata);
                        DWORD cmd = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                                                     menuX, menuY, parent, NULL);

                        char buf[100];
                        char bufcaption[100];
                        DWORD messagereturn = 0;
                        bool refreshPanel = false;

                        if (cmd != 0)
                        {
                                if (cmd >= 1000)
                                {
                                        BOOL enabled;
                                        int salamanderCmdType;
                                        if (SalamanderGeneral->GetSalamanderCommand(cmd - 1000, nameBuf, 200, &enabled,
                                                                                     &salamanderCmdType))
                                        {
                                                SalamanderGeneral->PostSalamanderCommand(cmd - 1000);
                                        }
                                }
                                else
                                {
                                        switch (cmd)
                                        {
                                        case MENUCMD_REGISTER:
                                                if (RegisterNewService(parent))
                                                        refreshPanel = true;
                                                break;
                                        case MENUCMD_START:
                                                if (RunServiceAction(parent, FSIdata->ServiceName, FSIdata->DisplayName,
                                                                     ServiceActionStart))
                                                        refreshPanel = true;
                                                break;
                                        case MENUCMD_STOP:
                                                if (RunServiceAction(parent, FSIdata->ServiceName, FSIdata->DisplayName,
                                                                     ServiceActionStop))
                                                        refreshPanel = true;
                                                break;
                                        case MENUCMD_PAUSE:
                                                if (RunServiceAction(parent, FSIdata->ServiceName, FSIdata->DisplayName,
                                                                     ServiceActionPause))
                                                        refreshPanel = true;
                                                break;
                                        case MENUCMD_RESUME:
                                                if (RunServiceAction(parent, FSIdata->ServiceName, FSIdata->DisplayName,
                                                                     ServiceActionResume))
                                                        refreshPanel = true;
                                                break;
                                        case MENUCMD_RESTART:
                                                if (RunServiceAction(parent, FSIdata->ServiceName, FSIdata->DisplayName,
                                                                     ServiceActionRestart))
                                                        refreshPanel = true;
                                                break;
                                        case MENUCMD_DELETE:
                                                if (!showDelete)
                                                        break;
                                                _snprintf(buf, 100, LoadStr(IDS_SERVICE_DELETE_CONFIRMATION),
                                                          FSIdata->DisplayName);
                                                _snprintf(bufcaption, 100, LoadStr(IDS_IDS_SERVICE_DELETE_DLG_CAPTION),
                                                          FSIdata->DisplayName);
                                                messagereturn = SalamanderGeneral->SalMessageBox(parent, buf, bufcaption,
                                                                                                MB_YESNO | MB_ICONQUESTION);
                                                if (messagereturn == IDYES)
                                                {
                                                        returnstate = DoDeleteSvc(FSIdata->ServiceName);
                                                        SalamanderGeneral->ToLowerCase(FSIdata->ServiceName);
                                                        SalamanderGeneral->RemoveOneFileFromCache(FSIdata->ServiceName);
                                                        SalamanderGeneral->PostChangeOnPathNotification("svc\\", false);
                                                        refreshPanel = (returnstate == 0);
                                                        if (returnstate > 0)
                                                        {
                                                                char errormessage[100];
                                                                switch (returnstate)
                                                                {
                                                                case 5:
                                                                        strcpy(errormessage, LoadStr(IDS_SERVICE_ERROR_INSUFFICIENTRIGHTS));
                                                                        break;
                                                                case 1072:
                                                                        strcpy(errormessage, LoadStr(IDS_SERVICE_ERROR_MARKEDFORDELETION));
                                                                        break;
                                                                default:
                                                                        strcpy(errormessage, LoadStr(IDS_SERVICE_ERROR_UNKNOWN));
                                                                }
                                                                char errBuf[500];
                                                                errBuf[499] = 0;
                                                                _snprintf(errBuf, 500, "%s\n\n%s %d: %s",
                                                                          LoadStr(IDS_SERVICE_ERROR_OPERATION),
                                                                          LoadStr(IDS_SEVICE_ERROR_CODE), returnstate,
                                                                          errormessage);
                                                                SalamanderGeneral->SalMessageBox(parent, errBuf,
                                                                                                  VERSINFO_PLUGINNAME,
                                                                                                  MB_OK | MB_ICONWARNING);
                                                                returnstate = 0;
                                                        }
                                                }
                                                break;
                                        case MENUCMD_PROPERTIES:
                                                {
                                                        char sbackupname[100];
                                                        strcpy(sbackupname, FSIdata->DisplayName);
                                                        if (dlg.Execute() == IDOK)
                                                        {
                                                                strcpy(FSIdata->DisplayName, sbackupname);
                                                                refreshPanel = true;
                                                        }
                                                }
                                                break;
                                        case MENUCMD_SCM:
                                                ShellExecute(0, "open", "services.msc", "", "", SW_SHOW);
                                                break;
                                        }
                                }
                        }

                        if (refreshPanel)
                                SalamanderGeneral->PostRefreshPanelFS(this);
                }
                break;
        default:
                break;
        }

        DestroyMenu(menu);

        SalamanderGeneral->PostChangeOnPathNotification("svc\\", false);
}

