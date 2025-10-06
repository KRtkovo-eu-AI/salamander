#include "precomp.h"
#include <windows.h>

CFSData *FSIGdata=NULL;
CTransferInfo tiG(NULL,ttDataToWindow);
int SelectedComboItem_CFG_PAGE1 = 0;
DWORD LastCfgPage = 0;   // start page (sheet) in configuration dialog

CCommonDialog::CCommonDialog(HINSTANCE hInstance, int resID, HWND hParent, CObjectOrigin origin)
: CDialog(hInstance, resID, hParent, origin)
{
}
CCommonDialog::CCommonDialog(HINSTANCE hInstance, int resID, int helpID, HWND hParent, CObjectOrigin origin)
: CDialog(hInstance, resID, helpID, hParent, origin)
{
}

INT_PTR CCommonDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      if (Parent != NULL)
        SalamanderGeneral->MultiMonCenterWindow(HWindow, Parent, TRUE);
      break; // chci focus od DefDlgProc
    }
  }
  return CDialog::DialogProc(uMsg, wParam, lParam);
}
CConfigPageFirst::CConfigPageFirst()
  : CPropSheetPage(NULL, GetLanguageResourceHandle(), IDD_CFGPAGEFIRST, IDD_CFGPAGEFIRST, PSP_HASHELP, NULL)
{
}

void CConfigPageFirst::Validate(CTransferInfo &ti)
{
	DWORD returnstate=0;
	HWND hWnd;
	if (ti.GetControl(hWnd, IDC_STATIC_CFG_STARTUPTYPE))
	{
		int SelectedComboItem_CFG_PAGE1_old = SelectedComboItem_CFG_PAGE1;
		SelectedComboItem_CFG_PAGE1 = static_cast<int>(SendMessage(hWnd, CB_GETCURSEL, 0, 0));

		if (SelectedComboItem_CFG_PAGE1_old!=SelectedComboItem_CFG_PAGE1)
		{
			switch (SelectedComboItem_CFG_PAGE1)
			{
				case 0:
					returnstate = ChangeSvc(FSIGdata->ServiceName,SVC_CHANGE_STARTTYPE,SVC_STARTTYPE_AUTO);
					break;
				case 1:
					returnstate = ChangeSvc(FSIGdata->ServiceName,SVC_CHANGE_STARTTYPE,SVC_STARTTYPE_ONDEMAND);
					break;
				case 2:
					returnstate = ChangeSvc(FSIGdata->ServiceName,SVC_CHANGE_STARTTYPE,SVC_STARTTYPE_DISABLED);
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
					default:
						strcpy(errormessage,LoadStr(IDS_SERVICE_ERROR_UNKNOWN));
				}
				char buf[500];
				buf[499] = 0;
				_snprintf(buf, 500, 
						"%s\n\n" 
						"%s %d: %s",LoadStr(IDS_SERVICE_ERROR_OPERATION),LoadStr(IDS_SEVICE_ERROR_CODE) ,returnstate,errormessage);
					SalamanderGeneral->SalMessageBox(HWindow, buf, VERSINFO_PLUGINNAME, MB_OK | MB_ICONWARNING);
			}

		}
	}
}
void CConfigPageFirst::EnableButtonStates(CTransferInfo &ti)
{
	DWORD ServiceStatus = GetServiceStatus(FSIGdata->ServiceName);
	FSIGdata->Status = ServiceStatus;

	HWND hWnd;
	ti.GetControl(hWnd, IDC_BUTTON_SERVICE_START);  EnableWindow(hWnd, FALSE);
	ti.GetControl(hWnd, IDC_BUTTON_SERVICE_STOP );  EnableWindow(hWnd, FALSE);
	ti.GetControl(hWnd, IDC_BUTTON_SERVICE_PAUSE);  EnableWindow(hWnd, FALSE);
	ti.GetControl(hWnd, IDC_BUTTON_SERVICE_RESUME); EnableWindow(hWnd, FALSE);
	switch (FSIGdata->Status)
	{
		case SERVICE_STOPPED:             
			ti.GetControl(hWnd, IDC_BUTTON_SERVICE_START);  EnableWindow(hWnd, TRUE);
			break;
		case SERVICE_START_PENDING:       break;
		case SERVICE_STOP_PENDING:        break;
		case SERVICE_RUNNING:             
			ti.GetControl(hWnd, IDC_BUTTON_SERVICE_STOP );  EnableWindow(hWnd, TRUE);
			break;
		case SERVICE_CONTINUE_PENDING:    break;
		case SERVICE_PAUSE_PENDING:       break;
		case SERVICE_PAUSED:              break;
		default:													break;
	}
	char Status[100];
	switch (FSIGdata->Status)
	{
		case SERVICE_STOPPED:             strcpy(Status, LoadStr(IDS_SERVICE_STATUS_STOPPED2)); break;
		case SERVICE_START_PENDING:       strcpy(Status, LoadStr(IDS_SERVICE_STATUS_STARTING)); break;
		case SERVICE_STOP_PENDING:        strcpy(Status, LoadStr(IDS_SERVICE_STATUS_STOPPING)); break;
		case SERVICE_RUNNING:             strcpy(Status, LoadStr(IDS_SERVICE_STATUS_RUNNING)); break;
		case SERVICE_CONTINUE_PENDING:    strcpy(Status, LoadStr(IDS_SERVICE_STATUS_CONTINUE_PENDING)); break;
		case SERVICE_PAUSE_PENDING:       strcpy(Status, LoadStr(IDS_SEVICE_STATUS_PAUSE_PENDING)); break;
		case SERVICE_PAUSED:              strcpy(Status, LoadStr(IDS_SERVICE_STATUS_PAUSED)); break;
		default:strcpy(Status, LoadStr(IDS_SERVICE_STATUS_STOPPED2));		break;
	}
	ti.EditLine(IDC_STATIC_CFG_STATUS, Status, static_cast<int>(strlen(Status)));
}

void CConfigPageFirst::EnableButtonStates()
{
	//HWND hParent = GetParent(Parent);
	//HWND hButton = ::GetDlgItem(hWnd,IDC_BUTTON_SERVICE_START);
}
void CConfigPageFirst::Transfer(CTransferInfo &ti)
{
	//SetWindowText ( HWindow, "New text here" );
	tiG = ti;

	EnableButtonStates(ti);
	//tiG = ti;
        const char *serviceName = FSIGdata->ServiceName != NULL ? FSIGdata->ServiceName : "";
        const char *displayNamePtr = FSIGdata->DisplayName != NULL ? FSIGdata->DisplayName : "";
        const char *executablePath = FSIGdata->ExecuteablePath != NULL ? FSIGdata->ExecuteablePath : "";

        ti.EditLine(IDC_STATIC_CFG_SERVICENAME, serviceName, static_cast<int>(strlen(serviceName)));
        ti.EditLine(IDC_STATIC_CFG_SERVICENAMET, serviceName, static_cast<int>(strlen(serviceName)));
        ti.EditLine(IDC_STATIC_CFG_DISPLAYNAME, displayNamePtr, static_cast<int>(strlen(displayNamePtr)));
        ti.EditLine(IDC_STATIC_CFG_EXECUTEABLEPATH, executablePath, static_cast<int>(strlen(executablePath)));

	char description[1000];
	char dependencies[1000];
	char displayname[1000];
	strcpy(description,"\0");
	strcpy(dependencies,"\0");
        strcpy(displayname,displayNamePtr);

	DoQuerySvc(FSIGdata->ServiceName, description,dependencies) ;

	if (strlen(description)>0)
		ti.EditLine(IDC_STATIC_CFG_DESCRIPTION, description, static_cast<int>(strlen(description)));
	

	char Status[100];
	switch (FSIGdata->Status)
	{
		case SERVICE_STOPPED:             strcpy(Status, LoadStr(IDS_SERVICE_STATUS_STOPPED2)); break;
		case SERVICE_START_PENDING:       strcpy(Status, LoadStr(IDS_SERVICE_STATUS_STARTING)); break;
		case SERVICE_STOP_PENDING:        strcpy(Status, LoadStr(IDS_SERVICE_STATUS_STOPPING)); break;
		case SERVICE_RUNNING:             strcpy(Status, LoadStr(IDS_SERVICE_STATUS_RUNNING)); break;
		case SERVICE_CONTINUE_PENDING:    strcpy(Status, LoadStr(IDS_SERVICE_STATUS_CONTINUE_PENDING)); break;
		case SERVICE_PAUSE_PENDING:       strcpy(Status, LoadStr(IDS_SEVICE_STATUS_PAUSE_PENDING)); break;
		case SERVICE_PAUSED:              strcpy(Status, LoadStr(IDS_SERVICE_STATUS_PAUSED)); break;
		default:strcpy(Status, LoadStr(IDS_SERVICE_STATUS_STOPPED2));		break;
	}
	ti.EditLine(IDC_STATIC_CFG_STATUS, Status, static_cast<int>(strlen(Status)));
	HWND hWnd;
	switch (FSIGdata->StartupType)
	{
		case SERVICE_AUTO_START:
			SelectedComboItem_CFG_PAGE1 = 0;	
			break;
		case SERVICE_DEMAND_START:
			SelectedComboItem_CFG_PAGE1 = 1;
			break;
		case SERVICE_DISABLED:
			SelectedComboItem_CFG_PAGE1 = 2;
			break;
		default:
			SelectedComboItem_CFG_PAGE1 = 0;
			break;
	}
  if (ti.GetControl(hWnd, IDC_STATIC_CFG_STARTUPTYPE))
  {
    if (ti.Type == ttDataToWindow)  // Transfer() volany pri otevirani okna (data -> okno)
    {
      SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
      SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)LoadStr(IDS_SERVICE_START_AUTO));
      SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)LoadStr(IDS_SERVICE_START_ONDEMAND));
      SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)LoadStr(IDS_SERVICE_START_DISABLED));
			SendMessage(hWnd, CB_SETCURSEL, SelectedComboItem_CFG_PAGE1, 0);
    }
    else   // ttDataFromWindow; Transfer() volany pri stisku OK (okno -> data)
    {
      SelectedComboItem_CFG_PAGE1 = static_cast<int>(SendMessage(hWnd, CB_GETCURSEL, 0, 0));
    }
  }
}


INT_PTR CConfigPageFirst::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD returnstate=0;
  switch (uMsg)
  {
	  break;
    case WM_COMMAND:
    {
			switch (LOWORD (wParam))
      {
				case IDC_BUTTON_SERVICE_START:
					SetCursor(LoadCursor(NULL,IDC_WAIT));
					ShowCursor(TRUE);
					returnstate = SStartService(FSIGdata->ServiceName);
					EnableButtonStates(tiG);
					ShowCursor(FALSE);
					SetCursor(LoadCursor(NULL,IDC_ARROW));
					break;
				case IDC_BUTTON_SERVICE_STOP:
					SetCursor(LoadCursor(NULL,IDC_WAIT));
					ShowCursor(TRUE);
					returnstate= SetStatus(FSIGdata->ServiceName,Stop);
					EnableButtonStates(tiG);
					ShowCursor(FALSE);
					SetCursor(LoadCursor(NULL,IDC_ARROW));
					break;
				case IDC_BUTTON_SERVICE_PAUSE:
					SetCursor(LoadCursor(NULL,IDC_WAIT));
					ShowCursor(TRUE);
					returnstate=SetStatus(FSIGdata->ServiceName,Pause);
					EnableButtonStates(tiG);
					ShowCursor(FALSE);
					SetCursor(LoadCursor(NULL,IDC_ARROW));
					break;
				case IDC_BUTTON_SERVICE_RESUME:
					SetCursor(LoadCursor(NULL,IDC_WAIT));
					ShowCursor(TRUE);
					returnstate=SetStatus(FSIGdata->ServiceName,Continue);
					EnableButtonStates(tiG);
					ShowCursor(FALSE);
					SetCursor(LoadCursor(NULL,IDC_ARROW));
					break;
				case IDC_BUTTON_SERVICE_DELETE:
					if(SalamanderGeneral->SalMessageBox(HWindow, "Do you really want to delete the current service?", VERSINFO_PLUGINNAME, MB_YESNOCANCEL | MB_ICONQUESTION)==IDYES)
					{
						SetCursor(LoadCursor(NULL,IDC_WAIT));
						ShowCursor(TRUE);
						returnstate=DoDeleteSvc(FSIGdata->ServiceName);
						EnableButtonStates(tiG);
						ShowCursor(FALSE);
						SetCursor(LoadCursor(NULL,IDC_ARROW));
					}
					break;

      }

      break; // chci focus od DefDlgProc
    }
  }
	if (returnstate>0)
	{
		
		char errormessage[100];
		switch (returnstate)
		{
			case 5:
				strcpy(errormessage,LoadStr(IDS_SERVICE_ERROR_INSUFFICIENTRIGHTS));
				break;
			default:
				strcpy(errormessage,LoadStr(IDS_SERVICE_ERROR_UNKNOWN));
		}
		char buf[500];
		buf[499] = 0;
	  _snprintf(buf, 500, 
        "%s\n\n" 
				"%s %d: %s",LoadStr(IDS_SERVICE_ERROR_OPERATION),LoadStr(IDS_SEVICE_ERROR_CODE) ,returnstate,errormessage);
			SalamanderGeneral->SalMessageBox(HWindow, buf, VERSINFO_PLUGINNAME, MB_OK | MB_ICONWARNING);
	}
	return CPropSheetPage::DialogProc(uMsg, wParam, lParam);
}


CConfigPageViewer::CConfigPageViewer()
  : CPropSheetPage(NULL, GetLanguageResourceHandle(), IDD_CFGPAGEVIEWER, IDD_CFGPAGEVIEWER, PSP_HASHELP, NULL)
{
}

void CConfigPageViewer::Transfer(CTransferInfo &ti)
{
	SalamanderGeneral->SalMessageBox(HWindow, "CConfigPageViewer-Transfer", "Error",MB_OK | MB_ICONEXCLAMATION);
}
class CCenteredPropertyWindow: public CWindow
{
  protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      switch (uMsg)
      {
        case WM_WINDOWPOSCHANGING:
        {
          WINDOWPOS *pos = (WINDOWPOS *)lParam;
          if (pos->flags & SWP_SHOWWINDOW)
          {
            HWND hParent = GetParent(HWindow);
            if (hParent != NULL)
              SalamanderGeneral->MultiMonCenterWindow(HWindow, hParent, TRUE);
          }
          break;
        }
        case WM_APP + 1000:   // mame se odpojit od dialogu (uz je vycentrovano)
        {
          DetachWindow();
          delete this;  // trochu prasarna, ale uz se 'this' nikdo ani nedotkne, takze pohoda
          return 0;
        }
				case WM_COMMAND:
				{
					//SalamanderGeneral->SalMessageBox(HWindow, "CConfigPageViewer-Transfer", "Error",MB_OK | MB_ICONEXCLAMATION);
				}
      }
      return CWindow::WindowProc(uMsg, wParam, lParam);
    }
};

#ifndef LPDLGTEMPLATEEX
#include <pshpack1.h>
typedef struct DLGTEMPLATEEX
{
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATEEX, *LPDLGTEMPLATEEX;
#include <poppack.h>
#endif // LPDLGTEMPLATEEX

// pomocny call-back pro centrovani konfiguracniho dialogu k parentovi a vyhozeni '?' buttonku z captionu
int CALLBACK CenterCallback(HWND HWindow, UINT uMsg, LPARAM lParam)
{
  if (uMsg == PSCB_INITIALIZED)   // pripojime se na dialog
  {

    CCenteredPropertyWindow *wnd = new CCenteredPropertyWindow;
    if (wnd != NULL)
    {
      wnd->AttachToWindow(HWindow);
      if (wnd->HWindow == NULL) delete wnd;  // okno neni pripojeny, zrusime ho uz tady
      else
      {
        PostMessage(wnd->HWindow, WM_APP + 1000, 0, 0);  // pro odpojeni CCenteredPropertyWindow od dialogu
      }
    }
  }
  if (uMsg == PSCB_PRECREATE)   // odstraneni '?' buttonku z headeru property sheetu
  {
    // Remove the DS_CONTEXTHELP style from the dialog box template
    if (((LPDLGTEMPLATEEX)lParam)->signature == 0xFFFF) ((LPDLGTEMPLATEEX)lParam)->style &= ~DS_CONTEXTHELP;
    else ((LPDLGTEMPLATE)lParam)->style &= ~DS_CONTEXTHELP;
  }
  return 0;
}

CConfigDialog::CConfigDialog(HWND parent,CFSData *FSITdata)
           : CPropertyDialog(parent, GetLanguageResourceHandle(), "Properties",
                             LastCfgPage, PSH_USECALLBACK | PSH_NOAPPLYNOW ,
                             NULL, &LastCfgPage, CenterCallback)
{
  Add(&PageFirst);
	FSIdata = FSITdata; 
	FSIGdata= FSITdata;
  //Add(&PageSecond);
  //Add(&PageViewer);
}
