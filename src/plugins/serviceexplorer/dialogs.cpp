#include "precomp.h"
#include <windows.h>
#include <ctype.h>
#include <cstring>

CFSData *FSIGdata=NULL;
int SelectedComboItem_CFG_PAGE1 = 0;
DWORD LastCfgPage = 0;   // start page (sheet) in configuration dialog

namespace
{
CTransferInfo &SharedTransferInfo()
{
    static CTransferInfo transferInfo(NULL, ttDataToWindow);
    return transferInfo;
}
}

void EnsureTransferInfoStorage()
{
    (void)SharedTransferInfo();
}

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

namespace
{
void SetReadOnlyText(CTransferInfo &ti, int ctrlID, const char *text)
{
    if (ti.Type != ttDataToWindow)
    {
        return;
    }

    HWND hWnd;
    if (ti.GetControl(hWnd, ctrlID))
    {
        const char *resolved = text != NULL ? text : "";
        SetWindowTextA(hWnd, resolved);
    }
}
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
        SharedTransferInfo() = ti;

        EnableButtonStates(ti);
        //SharedTransferInfo() = ti;
        const char *serviceName = FSIGdata->ServiceName != NULL ? FSIGdata->ServiceName : "";
        const char *displayNamePtr = FSIGdata->DisplayName != NULL ? FSIGdata->DisplayName : "";
        const char *executablePath = FSIGdata->ExecuteablePath != NULL ? FSIGdata->ExecuteablePath : "";

        SetReadOnlyText(ti, IDC_STATIC_CFG_SERVICENAME, serviceName);
        SetReadOnlyText(ti, IDC_STATIC_CFG_SERVICENAMET, serviceName);
        SetReadOnlyText(ti, IDC_STATIC_CFG_DISPLAYNAME, displayNamePtr);
        SetReadOnlyText(ti, IDC_STATIC_CFG_EXECUTEABLEPATH, executablePath);

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
  switch (uMsg)
  {
          break;
    case WM_COMMAND:
    {
			switch (LOWORD (wParam))
      {
                                case IDC_BUTTON_SERVICE_START:
                                        if (RunServiceAction(HWindow, FSIGdata->ServiceName, FSIGdata->DisplayName, ServiceActionStart))
                                                EnableButtonStates(SharedTransferInfo());
                                        break;
                                case IDC_BUTTON_SERVICE_STOP:
                                        if (RunServiceAction(HWindow, FSIGdata->ServiceName, FSIGdata->DisplayName, ServiceActionStop))
                                                EnableButtonStates(SharedTransferInfo());
                                        break;
                                case IDC_BUTTON_SERVICE_PAUSE:
                                        if (RunServiceAction(HWindow, FSIGdata->ServiceName, FSIGdata->DisplayName, ServiceActionPause))
                                                EnableButtonStates(SharedTransferInfo());
                                        break;
                                case IDC_BUTTON_SERVICE_RESUME:
                                        if (RunServiceAction(HWindow, FSIGdata->ServiceName, FSIGdata->DisplayName, ServiceActionResume))
                                                EnableButtonStates(SharedTransferInfo());
                                        break;
                                case IDC_BUTTON_SERVICE_DELETE:
                                        if(SalamanderGeneral->SalMessageBox(HWindow, "Do you really want to delete the current service?", VERSINFO_PLUGINNAME, MB_YESNOCANCEL | MB_ICONQUESTION)==IDYES)
                                        {
                                                SetCursor(LoadCursor(NULL,IDC_WAIT));
                                                ShowCursor(TRUE);
                                                DWORD returnstate=DoDeleteSvc(FSIGdata->ServiceName);
                                                EnableButtonStates(SharedTransferInfo());
                                                ShowCursor(FALSE);
                                                SetCursor(LoadCursor(NULL,IDC_ARROW));
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
                                                        SalamanderGeneral->SalMessageBox(HWindow, buf, VERSINFO_PLUGINNAME, MB_OK | MB_ICONWARNING);
                                                }
                                        }
                                        break;

      }

      break; // chci focus od DefDlgProc
    }
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

namespace
{
void TrimWhitespace(char* text)
{
    if (text == NULL)
        return;

    char* start = text;
    while (*start != 0 && isspace(static_cast<unsigned char>(*start)))
        ++start;

    if (start != text)
        memmove(text, start, strlen(start) + 1);

    size_t len = strlen(text);
    while (len > 0)
    {
        unsigned char ch = static_cast<unsigned char>(text[len - 1]);
        if (!isspace(ch))
            break;
        text[len - 1] = 0;
        --len;
    }
}

const char* ResolveString(int id, const char* fallback)
{
    const char* str = LoadStr(id);
    return (str != NULL && str[0] != 0) ? str : fallback;
}

void ToggleControlVisibility(HWND dialog, int controlID, BOOL visible)
{
    HWND ctrl = GetDlgItem(dialog, controlID);
    if (ctrl != NULL)
    {
        ShowWindow(ctrl, visible ? SW_SHOW : SW_HIDE);
        EnableWindow(ctrl, visible);
    }
}
} // namespace

class CRegisterServiceDialog : public CCommonDialog
{
  public:
    CRegisterServiceDialog(HWND parent, RegisterServiceConfig& cfg)
        : CCommonDialog(DLLInstance, IDD_REGISTER_SERVICE, parent)
        , Config(cfg)
    {
    }

  protected:
    virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

  private:
    void InitializeControls();
    void UpdateAccountControls();
    bool BrowseForExecutable();
    bool ValidateAndStore();

    RegisterServiceConfig& Config;
};

void CRegisterServiceDialog::InitializeControls()
{
    SendDlgItemMessage(HWindow, IDC_REGISTER_DISPLAYNAME, EM_SETLIMITTEXT, _countof(Config.DisplayName) - 1, 0);
    SendDlgItemMessage(HWindow, IDC_REGISTER_SERVICENAME, EM_SETLIMITTEXT, _countof(Config.ServiceName) - 1, 0);
    SendDlgItemMessage(HWindow, IDC_REGISTER_BINARY_PATH, EM_SETLIMITTEXT, _countof(Config.BinaryPath) - 1, 0);
    SendDlgItemMessage(HWindow, IDC_REGISTER_ARGUMENTS, EM_SETLIMITTEXT, _countof(Config.Arguments) - 1, 0);
    SendDlgItemMessage(HWindow, IDC_REGISTER_ACCOUNT_NAME, EM_SETLIMITTEXT, _countof(Config.CustomAccount) - 1, 0);
    SendDlgItemMessage(HWindow, IDC_REGISTER_PASSWORD, EM_SETLIMITTEXT, _countof(Config.Password) - 1, 0);

    SendDlgItemMessage(HWindow, IDC_REGISTER_STARTTYPE, CB_RESETCONTENT, 0, 0);
    SendDlgItemMessage(HWindow, IDC_REGISTER_STARTTYPE, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(ResolveString(IDS_SERVICE_START_AUTO, "Auto")));
    SendDlgItemMessage(HWindow, IDC_REGISTER_STARTTYPE, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(ResolveString(IDS_SERVICE_START_ONDEMAND, "Manual")));
    SendDlgItemMessage(HWindow, IDC_REGISTER_STARTTYPE, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(ResolveString(IDS_SERVICE_START_DISABLED, "Disabled")));

    int startIndex = 1;
    switch (Config.StartType)
    {
    case SERVICE_AUTO_START:
        startIndex = 0;
        break;
    case SERVICE_DISABLED:
        startIndex = 2;
        break;
    case SERVICE_DEMAND_START:
    default:
        startIndex = 1;
        break;
    }
    SendDlgItemMessage(HWindow, IDC_REGISTER_STARTTYPE, CB_SETCURSEL, startIndex, 0);

    SendDlgItemMessage(HWindow, IDC_REGISTER_ACCOUNT, CB_RESETCONTENT, 0, 0);
    SendDlgItemMessage(HWindow, IDC_REGISTER_ACCOUNT, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(ResolveString(IDS_REGISTER_ACCOUNT_LOCALSYSTEM, "Local System account")));
    SendDlgItemMessage(HWindow, IDC_REGISTER_ACCOUNT, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(ResolveString(IDS_REGISTER_ACCOUNT_LOCALSERVICE, "Local Service")));
    SendDlgItemMessage(HWindow, IDC_REGISTER_ACCOUNT, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(ResolveString(IDS_REGISTER_ACCOUNT_NETWORKSERVICE, "Network Service")));
    SendDlgItemMessage(HWindow, IDC_REGISTER_ACCOUNT, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(ResolveString(IDS_REGISTER_ACCOUNT_CUSTOM, "This account")));

    int accountIndex = static_cast<int>(Config.Account);
    if (accountIndex < 0 || accountIndex > RegisterServiceConfig::AccountCustom)
        accountIndex = RegisterServiceConfig::AccountLocalSystem;
    SendDlgItemMessage(HWindow, IDC_REGISTER_ACCOUNT, CB_SETCURSEL, accountIndex, 0);

    SetDlgItemTextA(HWindow, IDC_REGISTER_DISPLAYNAME, Config.DisplayName);
    SetDlgItemTextA(HWindow, IDC_REGISTER_SERVICENAME, Config.ServiceName);
    SetDlgItemTextA(HWindow, IDC_REGISTER_BINARY_PATH, Config.BinaryPath);
    SetDlgItemTextA(HWindow, IDC_REGISTER_ARGUMENTS, Config.Arguments);
    SetDlgItemTextA(HWindow, IDC_REGISTER_ACCOUNT_NAME, Config.CustomAccount);
    SetDlgItemTextA(HWindow, IDC_REGISTER_PASSWORD, Config.Password);

    CheckDlgButton(HWindow, IDC_REGISTER_START_IMMEDIATELY,
                   Config.StartAfterCreate ? BST_CHECKED : BST_UNCHECKED);

    UpdateAccountControls();
}

void CRegisterServiceDialog::UpdateAccountControls()
{
    int selection = static_cast<int>(SendDlgItemMessage(HWindow, IDC_REGISTER_ACCOUNT, CB_GETCURSEL, 0, 0));
    if (selection == CB_ERR)
        selection = RegisterServiceConfig::AccountLocalSystem;

    BOOL showCustom = (selection == RegisterServiceConfig::AccountCustom);
    ToggleControlVisibility(HWindow, IDC_REGISTER_ACCOUNT_LABEL, showCustom);
    ToggleControlVisibility(HWindow, IDC_REGISTER_ACCOUNT_NAME, showCustom);
    ToggleControlVisibility(HWindow, IDC_REGISTER_PASSWORD_LABEL, showCustom);
    ToggleControlVisibility(HWindow, IDC_REGISTER_PASSWORD, showCustom);

    if (!showCustom)
    {
        SetDlgItemTextA(HWindow, IDC_REGISTER_ACCOUNT_NAME, "");
        SetDlgItemTextA(HWindow, IDC_REGISTER_PASSWORD, "");
    }
}

bool CRegisterServiceDialog::BrowseForExecutable()
{
    char initialPath[_countof(Config.BinaryPath)];
    initialPath[0] = 0;
    GetDlgItemTextA(HWindow, IDC_REGISTER_BINARY_PATH, initialPath, _countof(initialPath));

    char fileBuffer[_countof(Config.BinaryPath)];
    lstrcpynA(fileBuffer, initialPath, _countof(fileBuffer));

    char title[128];
    lstrcpynA(title, ResolveString(IDS_REGISTER_BROWSE_TITLE, "Select Service Executable"), _countof(title));

    static const char filter[] = "Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0\0";

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = HWindow;
    ofn.hInstance = DLLInstance;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = _countof(fileBuffer);
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn))
    {
        SetDlgItemTextA(HWindow, IDC_REGISTER_BINARY_PATH, fileBuffer);
        return true;
    }
    return false;
}

bool CRegisterServiceDialog::ValidateAndStore()
{
    char serviceName[_countof(Config.ServiceName)];
    GetDlgItemTextA(HWindow, IDC_REGISTER_SERVICENAME, serviceName, _countof(serviceName));
    TrimWhitespace(serviceName);
    if (serviceName[0] == 0)
    {
        const char* message = ResolveString(IDS_REGISTER_ERROR_NO_SERVICE_NAME, "Please enter a service name.");
        SalamanderGeneral->SalMessageBox(HWindow, message, VERSINFO_PLUGINNAME, MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(HWindow, IDC_REGISTER_SERVICENAME));
        return false;
    }

    char displayName[_countof(Config.DisplayName)];
    GetDlgItemTextA(HWindow, IDC_REGISTER_DISPLAYNAME, displayName, _countof(displayName));
    TrimWhitespace(displayName);

    char binaryPath[_countof(Config.BinaryPath)];
    GetDlgItemTextA(HWindow, IDC_REGISTER_BINARY_PATH, binaryPath, _countof(binaryPath));
    TrimWhitespace(binaryPath);
    if (binaryPath[0] == 0)
    {
        const char* message = ResolveString(IDS_REGISTER_ERROR_NO_BINARY, "Please select an executable to run as a service.");
        SalamanderGeneral->SalMessageBox(HWindow, message, VERSINFO_PLUGINNAME, MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(HWindow, IDC_REGISTER_BINARY_PATH));
        return false;
    }

    size_t binaryLen = strlen(binaryPath);
    if (binaryLen >= 2 && binaryPath[0] == '"' && binaryPath[binaryLen - 1] == '"')
    {
        memmove(binaryPath, binaryPath + 1, binaryLen - 2);
        binaryPath[binaryLen - 2] = 0;
    }

    char absolutePath[_countof(Config.BinaryPath)];
    char* filePart = NULL;
    DWORD resolved = GetFullPathNameA(binaryPath, _countof(absolutePath), absolutePath, &filePart);
    if (resolved == 0 || resolved >= _countof(absolutePath))
        lstrcpynA(absolutePath, binaryPath, _countof(absolutePath));

    DWORD attrs = GetFileAttributesA(absolutePath);
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        const char* message = ResolveString(IDS_REGISTER_ERROR_INVALID_BINARY, "The specified executable could not be found.");
        SalamanderGeneral->SalMessageBox(HWindow, message, VERSINFO_PLUGINNAME, MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(HWindow, IDC_REGISTER_BINARY_PATH));
        return false;
    }

    char arguments[_countof(Config.Arguments)];
    GetDlgItemTextA(HWindow, IDC_REGISTER_ARGUMENTS, arguments, _countof(arguments));
    TrimWhitespace(arguments);

    int startSelection = static_cast<int>(SendDlgItemMessage(HWindow, IDC_REGISTER_STARTTYPE, CB_GETCURSEL, 0, 0));
    if (startSelection == CB_ERR)
        startSelection = 1;

    DWORD startType = SERVICE_DEMAND_START;
    switch (startSelection)
    {
    case 0:
        startType = SERVICE_AUTO_START;
        break;
    case 2:
        startType = SERVICE_DISABLED;
        break;
    case 1:
    default:
        startType = SERVICE_DEMAND_START;
        break;
    }

    int accountSelection = static_cast<int>(SendDlgItemMessage(HWindow, IDC_REGISTER_ACCOUNT, CB_GETCURSEL, 0, 0));
    if (accountSelection == CB_ERR)
        accountSelection = RegisterServiceConfig::AccountLocalSystem;

    char accountName[_countof(Config.CustomAccount)];
    accountName[0] = 0;
    char password[_countof(Config.Password)];
    password[0] = 0;

    if (accountSelection == RegisterServiceConfig::AccountCustom)
    {
        GetDlgItemTextA(HWindow, IDC_REGISTER_ACCOUNT_NAME, accountName, _countof(accountName));
        TrimWhitespace(accountName);
        if (accountName[0] == 0)
        {
            const char* message = ResolveString(IDS_REGISTER_ERROR_NO_ACCOUNT, "Please enter the account name.");
            SalamanderGeneral->SalMessageBox(HWindow, message, VERSINFO_PLUGINNAME, MB_OK | MB_ICONWARNING);
            SetFocus(GetDlgItem(HWindow, IDC_REGISTER_ACCOUNT_NAME));
            return false;
        }

        GetDlgItemTextA(HWindow, IDC_REGISTER_PASSWORD, password, _countof(password));
        if (password[0] == 0)
        {
            const char* message = ResolveString(IDS_REGISTER_ERROR_NO_PASSWORD, "Please enter the account password.");
            SalamanderGeneral->SalMessageBox(HWindow, message, VERSINFO_PLUGINNAME, MB_OK | MB_ICONWARNING);
            SetFocus(GetDlgItem(HWindow, IDC_REGISTER_PASSWORD));
            return false;
        }
    }

    lstrcpynA(Config.ServiceName, serviceName, _countof(Config.ServiceName));
    lstrcpynA(Config.DisplayName, (displayName[0] != 0) ? displayName : serviceName, _countof(Config.DisplayName));
    lstrcpynA(Config.BinaryPath, absolutePath, _countof(Config.BinaryPath));
    lstrcpynA(Config.Arguments, arguments, _countof(Config.Arguments));
    Config.StartType = startType;
    Config.Account = static_cast<RegisterServiceConfig::AccountKind>(accountSelection);
    lstrcpynA(Config.CustomAccount, accountName, _countof(Config.CustomAccount));
    lstrcpynA(Config.Password, password, _countof(Config.Password));
    Config.StartAfterCreate = (IsDlgButtonChecked(HWindow, IDC_REGISTER_START_IMMEDIATELY) == BST_CHECKED) ? TRUE : FALSE;

    return true;
}

INT_PTR CRegisterServiceDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        InitializeControls();
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_REGISTER_BROWSE:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                BrowseForExecutable();
                return TRUE;
            }
            break;
        case IDC_REGISTER_ACCOUNT:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                UpdateAccountControls();
                return TRUE;
            }
            break;
        case IDOK:
            if (ValidateAndStore())
                EndDialog(HWindow, IDOK);
            return TRUE;
        case IDCANCEL:
            EndDialog(HWindow, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

bool ShowRegisterServiceDialog(HWND parent, RegisterServiceConfig& config)
{
    CRegisterServiceDialog dlg(parent, config);
    return dlg.Execute() == IDOK;
}
