#include "precomp.h"
#include "dbg.h"
#include <limits.h>


//Customize the Columns

CFSData *FSdata;

const CFileData **TransferFileData = NULL;
int              *TransferIsDir = NULL;
char             *TransferBuffer = NULL;
int              *TransferLen = NULL;
DWORD            *TransferRowData = NULL;
CPluginDataInterfaceAbstract **TransferPluginDataIface = NULL;
DWORD            *TransferActCustomData = NULL;
// -----------------------------------------------------------------------------------------------------------
// Callback Functions
// -----------------------------------------------------------------------------------------------------------
void WINAPI GetTypeText()
{
        FSdata = (CFSData *)((*TransferFileData)->PluginData);
        const char *description = FSdata->Description != NULL ? FSdata->Description : "";
        const size_t len = strlen(description);
        *TransferLen = static_cast<int>(len > static_cast<size_t>(INT_MAX) ? INT_MAX : len);
        memcpy(TransferBuffer, description, static_cast<size_t>(*TransferLen));
}
void WINAPI GetStartupTypeText()
{
        FSdata = (CFSData *)((*TransferFileData)->PluginData);
        char StartupTyp[200];
        switch ( FSdata->StartupType)
  {
    case SERVICE_BOOT_START:        strcpy(StartupTyp, LoadStr(IDS_SERVICE_START_BOOT)); break;
    case SERVICE_SYSTEM_START:      strcpy(StartupTyp, LoadStr(IDS_SERVICE_START_SYSTEM)); break;
    case SERVICE_AUTO_START:        strcpy(StartupTyp, LoadStr(IDS_SERVICE_START_AUTO)); break;
    case SERVICE_DEMAND_START:                  strcpy(StartupTyp, LoadStr(IDS_SERVICE_START_ONDEMAND)); break;
    case SERVICE_DISABLED:                                      strcpy(StartupTyp, LoadStr(IDS_SERVICE_START_DISABLED)); break;
        default:
                StartupTyp[0] = 0;
                StartupTyp[1] = 0;
                break;
  }
  const size_t len = strlen(StartupTyp);
  *TransferLen = static_cast<int>(len > static_cast<size_t>(INT_MAX) ? INT_MAX : len);
  memcpy(TransferBuffer, StartupTyp, static_cast<size_t>(*TransferLen));
}

void WINAPI GetStatusTypeText()
{
        FSdata = (CFSData *)((*TransferFileData)->PluginData);
        char Status[200]="";

        switch ( FSdata->Status)
  {
    case SERVICE_STOPPED:             strcpy(Status, LoadStr(IDS_SERVICE_STATUS_STOPPED)); break;
    case SERVICE_START_PENDING:       strcpy(Status, LoadStr(IDS_SERVICE_STATUS_STARTING)); break;
    case SERVICE_STOP_PENDING:        strcpy(Status, LoadStr(IDS_SERVICE_STATUS_STOPPING)); break;
    case SERVICE_RUNNING:             strcpy(Status, LoadStr(IDS_SERVICE_STATUS_RUNNING)); break;
    case SERVICE_CONTINUE_PENDING:    strcpy(Status, LoadStr(IDS_SERVICE_STATUS_CONTINUE_PENDING)); break;
    case SERVICE_PAUSE_PENDING:       strcpy(Status, LoadStr(IDS_SEVICE_STATUS_PAUSE_PENDING)); break;
    case SERVICE_PAUSED:              strcpy(Status, LoadStr(IDS_SERVICE_STATUS_PAUSED)); break;
  }
        const size_t len = strlen(Status);
        *TransferLen = static_cast<int>(len > static_cast<size_t>(INT_MAX) ? INT_MAX : len);
        memcpy(TransferBuffer, Status, static_cast<size_t>(*TransferLen));
}
void WINAPI GetLogonAsText()
{
        FSdata = (CFSData *)((*TransferFileData)->PluginData);
        if (FSdata->LogOnAs !=NULL)
        {
                const size_t len = strlen(FSdata->LogOnAs);
                *TransferLen = static_cast<int>(len > static_cast<size_t>(INT_MAX) ? INT_MAX : len);
                memcpy(TransferBuffer, FSdata->LogOnAs, static_cast<size_t>(*TransferLen));
        }
        else
        {
                *TransferLen = 0;
        }
}
int WINAPI PluginSimpleIconCallback()
{
	return 0;
  //return *TransferIsDir ? 0 : 1;
}
// -----------------------------------------------------------------------------------------------------------
// Columns
// -----------------------------------------------------------------------------------------------------------
void AddDescriptionColumn(BOOL leftPanel, CSalamanderViewAbstract *view, int &i)
{
	int DFSTypeWidth = 0;
	int DFSTypeFixedWidth = 0;
	BOOL error=false;

	CColumn column;
  //strcpy(column.Name, "Description");
	strcpy(column.Name, LoadStr(IDS_COLUMN_CAPTION_DESCRIPTION));
  column.GetText = GetTypeText;
  column.CustomData = 1;
	column.LeftAlignment = 1;
  column.ID = COLUMN_ID_CUSTOM;
  column.Width = leftPanel ? LOWORD(DFSTypeWidth) : HIWORD(DFSTypeWidth);
  column.FixedWidth = leftPanel ? LOWORD(DFSTypeFixedWidth) : HIWORD(DFSTypeFixedWidth);
  error = view->InsertColumn(++i, &column);  // vlozime nas sloupec Type za originalni sloupec Type
}
void AddStartupTypeColumn(BOOL leftPanel, CSalamanderViewAbstract *view, int &i)
{
	int DFSTypeWidth = 30;
	int DFSTypeFixedWidth = 30;
	BOOL error=false;


  CColumn column;
  //strcpy(column.Name, "Startup Type");
	strcpy(column.Name, LoadStr(IDS_COLUMN_CAPTION_STARTUPTYPE));
  column.GetText = GetStartupTypeText;
  column.CustomData = 2;
	column.LeftAlignment = 1;
        column.SupportSorting = 1;
  column.ID = COLUMN_ID_CUSTOM;
  column.Width = leftPanel ? LOWORD(DFSTypeWidth) : HIWORD(DFSTypeWidth);
  column.FixedWidth = leftPanel ? LOWORD(DFSTypeFixedWidth) : HIWORD(DFSTypeFixedWidth);
  error = view->InsertColumn(++i, &column);  // vlozime nas sloupec Type za originalni sloupec Type
}
void AddStatusColumn(BOOL leftPanel, CSalamanderViewAbstract *view, int &i)
{
	int DFSTypeWidth = 0;
	int DFSTypeFixedWidth = 0;
	BOOL error=false;

	CColumn column;
  //strcpy(column.Name, "Status");
	strcpy(column.Name, LoadStr(IDS_COLUMN_CAPTION_STATUS));
  column.GetText = GetStatusTypeText;
  column.CustomData = 3;
	column.LeftAlignment = 1;
        column.SupportSorting = 1;
  column.ID = COLUMN_ID_CUSTOM;
  column.Width = leftPanel ? LOWORD(DFSTypeWidth) : HIWORD(DFSTypeWidth);
  column.FixedWidth = leftPanel ? LOWORD(DFSTypeFixedWidth) : HIWORD(DFSTypeFixedWidth);
  error = view->InsertColumn(++i, &column);  // vlozime nas sloupec Type za originalni sloupec Type
}
void AddLogOnAsColumn(BOOL leftPanel, CSalamanderViewAbstract *view, int &i)
{
	int DFSTypeWidth = 0;
	int DFSTypeFixedWidth = 0;
	BOOL error=false;

	CColumn column;
  //strcpy(column.Name, "Log On As");
	strcpy(column.Name, LoadStr(IDS_COLUMN_CAPTION_LOGONAS));
  column.GetText = GetLogonAsText;
  column.CustomData = 4;
	column.LeftAlignment = 1;
        column.SupportSorting = 1;
  column.ID = COLUMN_ID_CUSTOM;
  column.Width = leftPanel ? LOWORD(DFSTypeWidth) : HIWORD(DFSTypeWidth);
  column.FixedWidth = leftPanel ? LOWORD(DFSTypeFixedWidth) : HIWORD(DFSTypeFixedWidth);
  error = view->InsertColumn(++i, &column);  // vlozime nas sloupec Type za originalni sloupec Type
}
// -----------------------------------------------------------------------------------------------------------
// CPluginFSDataInterface
// -----------------------------------------------------------------------------------------------------------

CPluginFSDataInterface::CPluginFSDataInterface(const char *path)
{
  strcpy(Path, path);
  SalamanderGeneral->SalPathAddBackslash(Path, MAX_PATH);
  Name = Path + strlen(Path);
}

HIMAGELIST WINAPI CPluginFSDataInterface::GetSimplePluginIcons(int iconSize)
{
	return DFSImageList;
}

BOOL WINAPI CPluginFSDataInterface::HasSimplePluginIcon(CFileData &file, BOOL isDir)
{
        return TRUE;
}

HICON WINAPI
CPluginFSDataInterface::GetPluginIcon(const CFileData *file, int iconSize, BOOL &destroyIcon)
{
	////ImageList_GetIcon(DFSImageList,1,ILD_TRANSPARENT);
	//return ImageList_GetIcon(DFSImageList,0,ILC_MASK | SalamanderGeneral->GetImageListColorFlags());

  const ptrdiff_t offset = Name - Path;
  const ptrdiff_t remaining = MAX_PATH - offset;
  int copyLen = remaining > 0 ? static_cast<int>(remaining) : MAX_PATH;
  if (copyLen <= 0)
    copyLen = MAX_PATH;
  lstrcpyn(Name, file->Name, copyLen);
  BOOL isDir = (file->Attr & FILE_ATTRIBUTE_DIRECTORY) != 0;

  SHFILEINFO shi = {};
  if (!SalamanderGeneral->GetFileIcon(Path, FALSE, &shi.hIcon, iconSize, TRUE, isDir))
  {
    if (!SHGetFileInfo(Path, 0, &shi, sizeof(shi),
                       SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SHELLICONSIZE))
    {
      shi.hIcon = NULL;   // failure
    }
  }
  destroyIcon = TRUE;
  return shi.hIcon;   // icon or NULL (failure)
}

int WINAPI CPluginFSDataInterface::CompareFilesFromFS(const CFileData *file1, const CFileData *file2)
{
  if (file1 == NULL || file2 == NULL)
    return 0;

  DWORD custom = (TransferActCustomData != NULL) ? *TransferActCustomData : 0;
  const CFSData *data1 = reinterpret_cast<const CFSData *>(file1->PluginData);
  const CFSData *data2 = reinterpret_cast<const CFSData *>(file2->PluginData);

  int result = 0;
  switch (custom)
  {
  case 2: // Startup Type
    if (data1 != NULL && data2 != NULL)
    {
      if (data1->StartupType < data2->StartupType)
        result = -1;
      else if (data1->StartupType > data2->StartupType)
        result = 1;
    }
    break;
  case 3: // Status
    if (data1 != NULL && data2 != NULL)
    {
      if (data1->Status < data2->Status)
        result = -1;
      else if (data1->Status > data2->Status)
        result = 1;
    }
    break;
  case 4: // Log on As
    if (SalamanderGeneral != NULL)
    {
      const char *left = (data1 != NULL && data1->LogOnAs != NULL) ? data1->LogOnAs : "";
      const char *right = (data2 != NULL && data2->LogOnAs != NULL) ? data2->LogOnAs : "";
      result = SalamanderGeneral->StrICmp(left, right);
    }
    break;
  default:
    break;
  }

  if (result == 0)
  {
    if (SalamanderGeneral != NULL)
      result = SalamanderGeneral->StrICmp(file1->Name, file2->Name);
    else
      result = strcmp(file1->Name, file2->Name);
  }

  return result;
}

void WINAPI CPluginFSDataInterface::SetupView(BOOL leftPanel, CSalamanderViewAbstract *view, const char *archivePath,
                                  const CFileData *upperDir)
{
	view->GetTransferVariables(TransferFileData, TransferIsDir, TransferBuffer, TransferLen, TransferRowData,
														 TransferPluginDataIface, TransferActCustomData);
	
	view->SetPluginSimpleIconCallback(PluginSimpleIconCallback);

	if (view->GetViewMode() == VIEW_MODE_DETAILED)  // upravime sloupce
  {
    int count = view->GetColumnsCount();
;
    for (int i = 0; i < count; i++)
    {
      const CColumn *c = view->GetColumn(i);
			if (c->ID == COLUMN_ID_EXTENSION || c->ID == COLUMN_ID_DOSNAME  || c->ID == COLUMN_ID_SIZE   || 
					c->ID == COLUMN_ID_TYPE      || c->ID == COLUMN_ID_DATE     || c->ID ==  COLUMN_ID_TIME  || 
					c->ID == COLUMN_ID_ATTRIBUTES|| c->ID == COLUMN_ID_DESCRIPTION)
			{
				view->DeleteColumn(i);
				i--;
				count = view->GetColumnsCount();
			}
    }
		count = view->GetColumnsCount(); 
		int i = count -1;

		//AddDescriptionColumn(leftPanel,view,i);
		AddStatusColumn(leftPanel, view, i);
		AddStartupTypeColumn(leftPanel,view,i);
		AddLogOnAsColumn(leftPanel,view,i);
		count = view->GetColumnsCount();
	}
}

void WINAPI CPluginFSDataInterface::ColumnFixedWidthShouldChange(BOOL leftPanel, const CColumn *column, int newFixedWidth)
{
}

void WINAPI CPluginFSDataInterface::ColumnWidthWasChanged(BOOL leftPanel, const CColumn *column, int newWidth)
{
}

struct CFSInfoLineData
{
  const char *Name;
};
const char * WINAPI FSInfoLineFile(HWND parent, void *param)
{
  CFSInfoLineData *data = (CFSInfoLineData *)param;
  return data->Name;
}

CSalamanderVarStrEntry FSInfoLine[] =
{
  {"Service", FSInfoLineFile},
  {NULL}
};

BOOL WINAPI CPluginFSDataInterface::GetInfoLineContent(int panel, const CFileData *file, BOOL isDir,
                                           int selectedFiles, int selectedDirs, BOOL displaySize,
                                           const CQuadWord &selectedSize, char *buffer,
                                           DWORD *hotTexts, int &hotTextsCount)
{
	if (file !=NULL)
	{
		  CFSInfoLineData data;
			data.Name = file->Name;
			hotTextsCount = 100;
			if (!SalamanderGeneral->ExpandVarString(SalamanderGeneral->GetMsgBoxParent(),
				"Service: $(Service) ", buffer, 1000, FSInfoLine,
																							&data, FALSE, hotTexts, &hotTextsCount))
			{
				strcpy(buffer, "Error!");
				hotTextsCount = 0;
			}
		return TRUE;
	}
	else //Multiple items are selected ?
	{
		//sprintf (buffer, "<%d> selected services.",selectedFiles);
		char buf[1000];
		CQuadWord qwCount(selectedFiles, 0);
		SalamanderGeneral->ExpandPluralString (buf, 1000, "{!}%d selected service{|1|s}", 1,&qwCount );
		_snprintf(buffer,1000,buf,selectedFiles);
		return SalamanderGeneral->LookForSubTexts(buffer, hotTexts, &hotTextsCount);;
	}
}
