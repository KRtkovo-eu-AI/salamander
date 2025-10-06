#include "precomp.h"

// image-list for simple icons FS
HIMAGELIST DFSImageList = NULL;

char **History = NULL;
int HistoryCount = 0;

// FS-name given Salamander to load plugin
char AssignedFSName[MAX_PATH] = "";
extern int AssignedFSNameLen = 0;

// Temporary variable for tests
CPluginFSInterfaceAbstract *LastDetachedFS = NULL;

// structure for transmitting data from the "Connect" dialogue to the newly created FS
CConnectData ConnectData;

// ****************************************************************************
// SECTION FILE SYSTEM
// ****************************************************************************

BOOL InitFS()
{
	OutputDebugString ("InitFS");

  DFSImageList = ImageList_Create(16, 16, ILC_MASK | SalamanderGeneral->GetImageListColorFlags(), 2, 0);
  if (DFSImageList == NULL)
  {
    TRACE_E("Unable to create image list.");
    return FALSE;
  }
  ImageList_SetImageCount(DFSImageList, 2); // Initialization
  ImageList_SetBkColor(DFSImageList, SalamanderGeneral->GetCurrentColor(SALCOL_ITEM_BK_NORMAL));

// Icons are different on various Windows, query a small folder icon dynamically.
  HICON dirIcon = NULL;
  if (SalamanderGeneral->GetFileIcon("C:\\", FALSE, &dirIcon, SALICONSIZE_16, TRUE, TRUE) && dirIcon != NULL)
  {
    ImageList_ReplaceIcon(DFSImageList, 0, dirIcon);
    HANDLES(DestroyIcon(dirIcon));
  }
  else
  {
    HICON shared = HANDLES(LoadIcon(NULL, IDI_APPLICATION));
    if (shared != NULL)
    {
      HICON copy = HANDLES(CopyIcon(shared));
      if (copy != NULL)
      {
        ImageList_ReplaceIcon(DFSImageList, 0, copy);
        HANDLES(DestroyIcon(copy));
      }
    }
  }

        return TRUE;
}

void ReleaseFS()
{
	OutputDebugString("fs2-ReleaseFS");
	ImageList_Destroy(DFSImageList);
}

// ****************************************************************************
// CPluginInterfaceForFS
// ****************************************************************************

CPluginFSInterfaceAbstract * WINAPI CPluginInterfaceForFS::OpenFS(const char *fsName, int fsNameIndex)
{
	OutputDebugString("fs2-OpenFS");
	ActiveFSCount++;
	return new CPluginFSInterface;
}

void WINAPI CPluginInterfaceForFS::CloseFS(CPluginFSInterfaceAbstract *fs)
{
	OutputDebugString("fs2-CloseFS");

	CPluginFSInterface *dfsFS = (CPluginFSInterface *)fs;
	if (dfsFS == LastDetachedFS) LastDetachedFS = NULL;
  ActiveFSCount--;
  if (dfsFS != NULL) delete dfsFS;
}

void WINAPI CPluginInterfaceForFS::ExecuteChangeDriveMenuItem(int panel)
{
        (void)panel;

        SalamanderGeneral->GetStdHistoryValues(SALHIST_CHANGEDIR, &History, &HistoryCount);
        UpdateWindow(SalamanderGeneral->GetMainWindowHWND());

	int failReason;
    BOOL changeRes = SalamanderGeneral->ChangePanelPathToPluginFS(PANEL_SOURCE, AssignedFSName, "", &failReason);
  OutputDebugString("fs1-ExecuteChangeDriveMenuItem");
}

BOOL WINAPI CPluginInterfaceForFS::ChangeDriveMenuItemContextMenu(HWND parent, int panel, int x, int y,
                                                      CPluginFSInterfaceAbstract *pluginFS,
                                                      const char *pluginFSName, int pluginFSNameIndex,
                                                      BOOL isDetachedFS, BOOL &refreshMenu,
                                                      BOOL &closeMenu, int &postCmd, void *&postCmdParam)
{
        (void)panel;
        OutputDebugString("ChangeDriveMenuItemContextMenu");
        return FALSE;
}

void WINAPI CPluginInterfaceForFS::ExecuteChangeDrivePostCommand(int panel, int postCmd, void *postCmdParam)
{
        (void)panel;
        OutputDebugString("ExecuteChangeDrivePostCommand");

}

BOOL WINAPI CPluginInterfaceForFS::DisconnectFS(HWND parent, BOOL isInPanel, int panel,
                                    CPluginFSInterfaceAbstract *pluginFS,
                                    const char *pluginFSName, int pluginFSNameIndex)
{
	OutputDebugString("DisconnectFS");
	SalamanderGeneral->CloseDetachedFS(parent,pluginFS);
	return true;
}

void WINAPI CPluginInterfaceForFS::ExecuteOnFS(int panel, CPluginFSInterfaceAbstract *pluginFS,
                                   const char *pluginFSName, int pluginFSNameIndex,
                                   CFileData &file, int isDir)
{
	CFSData *FSIdata;
	FSIdata = (CFSData*) (file.PluginData);	//Plugin Item Data
	char sbackupname[100];
	strcpy(sbackupname,FSIdata->DisplayName);
	CConfigDialog dlg(SalamanderGeneral->GetMainWindowHWND(),FSIdata);
	if (dlg.Execute() == IDOK)
	{
		strcpy(FSIdata->DisplayName,sbackupname);
		SalamanderGeneral->PostRefreshPanelFS(pluginFS);
		//strcpy(FSIdata->DisplayName,sbackupname);
	}
	else
	{
	}

}
