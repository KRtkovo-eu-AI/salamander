#include "precomp.h"

// image-list for simple icons FS
HIMAGELIST DFSImageList = NULL;
static int g_CurrentImageListSize = 0;

int IconSizeToPixels(int iconSize)
{
  switch (iconSize)
  {
  case SALICONSIZE_48:
    return 48;
  case SALICONSIZE_32:
    return 32;
  default:
    return 16;
  }
}

BOOL EnsureServiceImageList(int iconSize)
{
  if (DFSImageList != NULL && g_CurrentImageListSize == iconSize)
    return TRUE;

  if (DFSImageList != NULL)
  {
    ImageList_Destroy(DFSImageList);
    DFSImageList = NULL;
    g_CurrentImageListSize = 0;
  }

  const int pixels = IconSizeToPixels(iconSize);
  HIMAGELIST list = ImageList_Create(pixels, pixels,
                                     ILC_COLOR32 | ILC_MASK, 1, 0);
  if (list == NULL)
    return FALSE;

  ImageList_SetImageCount(list, 1);
  HICON icon = reinterpret_cast<HICON>(LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_SERVICEEXPLORER_DIR),
                                                 IMAGE_ICON, pixels, pixels, LR_DEFAULTCOLOR));
  if (icon == NULL)
  {
    ImageList_Destroy(list);
    return FALSE;
  }

  ImageList_ReplaceIcon(list, 0, icon);
  HANDLES(DestroyIcon(icon));
  ImageList_SetBkColor(list, SalamanderGeneral->GetCurrentColor(SALCOL_ITEM_BK_NORMAL));

  DFSImageList = list;
  g_CurrentImageListSize = iconSize;
  return TRUE;
}

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

  if (!EnsureServiceImageList(SALICONSIZE_16))
  {
    TRACE_E("Unable to create service explorer icon list.");
    return FALSE;
  }

  return TRUE;
}

void ReleaseFS()
{
        OutputDebugString("fs2-ReleaseFS");
        if (DFSImageList != NULL)
        {
                ImageList_Destroy(DFSImageList);
                DFSImageList = NULL;
                g_CurrentImageListSize = 0;
        }
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
