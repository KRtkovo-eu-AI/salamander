#ifndef __DIALOGS_H
#define __DIALOGS_H

//Base Class
class CCommonDialog: public CDialog
{
  public:
    CCommonDialog(HINSTANCE hInstance, int resID, HWND hParent, CObjectOrigin origin = ooStandard);
    CCommonDialog(HINSTANCE hInstance, int resID, int helpID, HWND hParent, CObjectOrigin origin = ooStandard);

  protected:
    INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};


class CConfigPageFirst: public CPropSheetPage
{
  public:
    CConfigPageFirst();

    virtual void Validate(CTransferInfo &ti);  
    virtual void Transfer(CTransferInfo &ti);

	private:
		void EnableButtonStates(CTransferInfo &ti);
		void EnableButtonStates();
		

        protected:
                virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};


class CConfigPageViewer: public CPropSheetPage
{
  public:
    CConfigPageViewer();
    virtual void Transfer(CTransferInfo &ti);
};


class CConfigDialog: public CPropertyDialog
{
  protected:
    CConfigPageFirst PageFirst;
                CConfigPageViewer PageViewer;
                CFSData *FSIdata;
public:
    CConfigDialog(HWND parent,CFSData *FSITdata);
};

struct RegisterServiceConfig;
bool ShowRegisterServiceDialog(HWND parent, RegisterServiceConfig &config);
#endif //__DIALOGS_H
