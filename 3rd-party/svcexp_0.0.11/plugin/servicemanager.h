

#ifndef CServiceManagerH
#define CServiceManagerH


enum SvcCommandType
{
	SVC_CHANGE_STARTTYPE
};
enum SvcCommand
{
	SVC_STARTTYPE_AUTO,
	SVC_STARTTYPE_ONDEMAND,
	SVC_STARTTYPE_DISABLED
};

enum SControlType { Stop=0,Pause,Continue,Interrogate,Shutdown } ;
DWORD SetStatus (char *cSvcName, SControlType sct);
const char *GetLastErrorMessage();
DWORD GetServiceStatus(char *cSvcName);
QUERY_SERVICE_CONFIG *GetQueryServiceConfig(char *szService);
DWORD SStartService(char *cSvcName);
DWORD SGetStatus(char *cSvcName);
DWORD DoQuerySvc(char *szSvcName,char *szDescription, char *szDependencies);
DWORD ChangeSvc(char *szSvcName,SvcCommandType eCommandType, SvcCommand eCommand);
DWORD DoDeleteSvc(char *szSvcName);
#endif