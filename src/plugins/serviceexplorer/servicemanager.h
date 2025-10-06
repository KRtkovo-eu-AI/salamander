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

enum SControlType
{
    Stop = 0,
    Pause,
    Continue,
    Interrogate,
    Shutdown
};

DWORD SetStatus(char* cSvcName, SControlType sct);
const char* GetLastErrorMessage();
DWORD GetServiceStatus(char* cSvcName);
QUERY_SERVICE_CONFIG* GetQueryServiceConfig(char* szService);
DWORD SStartService(char* cSvcName);
DWORD SGetStatus(char* cSvcName);
DWORD DoQuerySvc(char* szSvcName, char* szDescription, char* szDependencies);
DWORD ChangeSvc(char* szSvcName, SvcCommandType eCommandType, SvcCommand eCommand);
DWORD DoDeleteSvc(char* szSvcName);

struct RegisterServiceConfig
{
    enum AccountKind
    {
        AccountLocalSystem,
        AccountLocalService,
        AccountNetworkService,
        AccountCustom
    };

    RegisterServiceConfig()
        : StartType(SERVICE_DEMAND_START)
        , Account(AccountLocalSystem)
        , StartAfterCreate(FALSE)
    {
        ServiceName[0] = 0;
        DisplayName[0] = 0;
        BinaryPath[0] = 0;
        Arguments[0] = 0;
        CustomAccount[0] = 0;
        Password[0] = 0;
    }

    char ServiceName[MAX_PATH];
    char DisplayName[256];
    char BinaryPath[MAX_PATH * 4];
    char Arguments[512];
    DWORD StartType;
    AccountKind Account;
    char CustomAccount[256];
    char Password[256];
    BOOL StartAfterCreate;
};

enum ServiceActionKind
{
    ServiceActionStart,
    ServiceActionStop,
    ServiceActionPause,
    ServiceActionResume,
    ServiceActionRestart
};

BOOL RunServiceAction(HWND parent, const char* serviceName, const char* displayName, ServiceActionKind action);
BOOL RegisterNewService(HWND parent);

#endif
