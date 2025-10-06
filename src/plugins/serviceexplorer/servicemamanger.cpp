#include "precomp.h"
#include "servicemanager.h"

#include <string>

namespace
{
struct ServiceActionInfo
{
    ServiceActionKind Action;
    DWORD DesiredState;
    DWORD PendingState;
    DWORD ControlCode;
    DWORD AccessMask;
    int ProgressTextRes;
    int FailureTextRes;
    int AlreadyTextRes;
};

struct ServiceActionOutcome
{
    ServiceActionOutcome()
        : ErrorCode(ERROR_SUCCESS)
        , ServiceSpecific(0)
        , ShowAlreadyMessage(FALSE)
        , Info(NULL)
    {
    }

    DWORD ErrorCode;
    DWORD ServiceSpecific;
    BOOL ShowAlreadyMessage;
    const ServiceActionInfo* Info;
};

struct ServiceActionWorkerContext
{
    ServiceActionWorkerContext(const char* service, const char* display, ServiceActionKind actionKind)
        : ServiceName(service != NULL ? service : "")
        , DisplayName(display != NULL ? display : "")
        , Action(actionKind)
        , CompletionEvent(CreateEvent(NULL, TRUE, FALSE, NULL))
        , Success(FALSE)
    {
    }

    ~ServiceActionWorkerContext()
    {
        if (CompletionEvent != NULL)
            CloseHandle(CompletionEvent);
    }

    std::string ServiceName;
    std::string DisplayName;
    ServiceActionKind Action;
    HANDLE CompletionEvent;
    ServiceActionOutcome Outcome;
    BOOL Success;
};

typedef HRESULT(WINAPI* PFN_TaskDialogIndirect)(const TASKDIALOGCONFIG*, int*, int*, BOOL*);

PFN_TaskDialogIndirect ResolveTaskDialog()
{
    static PFN_TaskDialogIndirect taskDialog = NULL;
    static bool resolved = false;
    if (!resolved)
    {
        resolved = true;
        HMODULE module = LoadLibraryW(L"comctl32.dll");
        if (module != NULL)
            taskDialog = reinterpret_cast<PFN_TaskDialogIndirect>(GetProcAddress(module, "TaskDialogIndirect"));
    }
    return taskDialog;
}

std::wstring AnsiToWide(const char* text)
{
    if (text == NULL)
        return std::wstring();
    int len = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    if (len <= 0)
        return std::wstring();
    std::wstring wide(static_cast<size_t>(len) - 1, L'\0');
    if (!wide.empty())
        MultiByteToWideChar(CP_ACP, 0, text, -1, &wide[0], len);
    return wide;
}

struct TaskDialogContext
{
    explicit TaskDialogContext(HANDLE completion)
        : Completion(completion)
        , CanClose(FALSE)
    {
    }

    HANDLE Completion;
    BOOL CanClose;
};

HRESULT CALLBACK ServiceActionTaskDialogCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR refData)
{
    TaskDialogContext* ctx = reinterpret_cast<TaskDialogContext*>(refData);
    switch (msg)
    {
    case TDN_CREATED:
        SendMessage(hwnd, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 0);
        SendMessage(hwnd, TDM_ENABLE_BUTTON, IDCANCEL, FALSE);
        break;
    case TDN_TIMER:
        if (ctx != NULL && ctx->Completion != NULL && WaitForSingleObject(ctx->Completion, 0) == WAIT_OBJECT_0)
        {
            ctx->CanClose = TRUE;
            SendMessage(hwnd, TDM_CLICK_BUTTON, IDCANCEL, 0);
        }
        break;
    case TDN_BUTTON_CLICKED:
        if (!ctx->CanClose)
            return S_FALSE;
        break;
    }
    return S_OK;
}

void PumpMessageLoopUntil(HANDLE completionEvent)
{
    if (completionEvent == NULL)
        return;
    HANDLE handles[1] = {completionEvent};
    while (true)
    {
        DWORD wait = MsgWaitForMultipleObjects(1, handles, FALSE, INFINITE, QS_ALLINPUT);
        if (wait == WAIT_OBJECT_0)
            break;
        if (wait == WAIT_OBJECT_0 + 1)
        {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    PostQuitMessage(static_cast<int>(msg.wParam));
                }
                else
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
        else
        {
            break;
        }
    }
}

const ServiceActionInfo ServiceActions[] = {
    {ServiceActionStart, SERVICE_RUNNING, SERVICE_START_PENDING, 0, SERVICE_START | SERVICE_QUERY_STATUS, IDS_SERVICE_PROGRESS_STARTING, IDS_SERVICE_ERROR_START_FAILED, IDS_SERVICE_ALREADY_RUNNING},
    {ServiceActionStop, SERVICE_STOPPED, SERVICE_STOP_PENDING, SERVICE_CONTROL_STOP, SERVICE_STOP | SERVICE_QUERY_STATUS, IDS_SERVICE_PROGRESS_STOPPING, IDS_SERVICE_ERROR_STOP_FAILED, IDS_SERVICE_ALREADY_STOPPED},
    {ServiceActionPause, SERVICE_PAUSED, SERVICE_PAUSE_PENDING, SERVICE_CONTROL_PAUSE, SERVICE_PAUSE_CONTINUE | SERVICE_QUERY_STATUS, IDS_SERVICE_PROGRESS_PAUSING, IDS_SERVICE_ERROR_PAUSE_FAILED, IDS_SERVICE_ALREADY_PAUSED},
    {ServiceActionResume, SERVICE_RUNNING, SERVICE_CONTINUE_PENDING, SERVICE_CONTROL_CONTINUE, SERVICE_PAUSE_CONTINUE | SERVICE_QUERY_STATUS, IDS_SERVICE_PROGRESS_RESUMING, IDS_SERVICE_ERROR_RESUME_FAILED, IDS_SERVICE_ALREADY_RUNNING}
};

void TrimTrailingWhitespace(char* text)
{
    if (text == NULL)
        return;
    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\r' || text[len - 1] == '\n' || text[len - 1] == ' ' || text[len - 1] == '\t'))
    {
        text[len - 1] = 0;
        len--;
    }
}

void FormatSystemErrorString(DWORD error, char* buffer, size_t bufferSize)
{
    if (bufferSize == 0)
        return;
    buffer[0] = 0;
    LPSTR local = NULL;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    DWORD len = FormatMessageA(flags, NULL, error, 0, reinterpret_cast<LPSTR>(&local), 0, NULL);
    if (len == 0)
    {
        HMODULE netMsg = LoadLibraryExA("netmsg.dll", NULL, DONT_RESOLVE_DLL_REFERENCES);
        if (netMsg != NULL)
        {
            len = FormatMessageA(flags | FORMAT_MESSAGE_FROM_HMODULE, netMsg, error, 0, reinterpret_cast<LPSTR>(&local), 0, NULL);
            FreeLibrary(netMsg);
        }
    }
    if (len != 0 && local != NULL)
    {
        lstrcpyn(buffer, local, static_cast<int>(bufferSize));
        buffer[bufferSize - 1] = 0;
        TrimTrailingWhitespace(buffer);
        LocalFree(local);
    }
}

void FormatActionString(int resID, const char* displayName, char* buffer, size_t bufferSize)
{
    const char* format = LoadStr(resID);
    if (format == NULL)
        format = "%s";
    const char* name = (displayName != NULL && displayName[0] != 0) ? displayName : "";
    _snprintf(buffer, bufferSize, format, name);
    buffer[bufferSize - 1] = 0;
}

BOOL QueryStatus(SC_HANDLE service, SERVICE_STATUS_PROCESS& status)
{
    DWORD bytesNeeded = 0;
    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, reinterpret_cast<BYTE*>(&status), sizeof(status), &bytesNeeded))
        return FALSE;
    return TRUE;
}

DWORD WaitForServiceState(SC_HANDLE service, const ServiceActionInfo& info, DWORD& serviceSpecific)
{
    serviceSpecific = 0;
    SERVICE_STATUS_PROCESS status;
    if (!QueryStatus(service, status))
        return GetLastError();

    if (status.dwCurrentState == info.DesiredState)
        return ERROR_SUCCESS;

    DWORD startTick = GetTickCount();
    DWORD oldCheckPoint = status.dwCheckPoint;

    while (status.dwCurrentState == info.PendingState)
    {
        DWORD waitTime = status.dwWaitHint / 10;
        if (waitTime < 1000)
            waitTime = 1000;
        else if (waitTime > 10000)
            waitTime = 10000;
        Sleep(waitTime);
        if (!QueryStatus(service, status))
            return GetLastError();
        if (status.dwCurrentState == info.DesiredState)
            return ERROR_SUCCESS;
        if (status.dwCheckPoint > oldCheckPoint)
        {
            oldCheckPoint = status.dwCheckPoint;
            startTick = GetTickCount();
        }
        else if (GetTickCount() - startTick > status.dwWaitHint)
            break;
    }

    if (status.dwCurrentState == info.DesiredState)
        return ERROR_SUCCESS;

    if (status.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR && status.dwServiceSpecificExitCode != 0)
        serviceSpecific = status.dwServiceSpecificExitCode;
    else if (status.dwServiceSpecificExitCode != 0 && status.dwWin32ExitCode == ERROR_SUCCESS)
        serviceSpecific = status.dwServiceSpecificExitCode;

    if (status.dwWin32ExitCode != ERROR_SUCCESS)
        return status.dwWin32ExitCode;

    if (status.dwCurrentState == info.PendingState)
        return ERROR_SERVICE_REQUEST_TIMEOUT;

    if (status.dwCurrentState == SERVICE_STOPPED && info.DesiredState != SERVICE_STOPPED && status.dwWin32ExitCode != ERROR_SUCCESS)
        return status.dwWin32ExitCode;

    return ERROR_SERVICE_REQUEST_TIMEOUT;
}

void ShowServiceOperationError(HWND parent, const char* displayName, const ServiceActionInfo& info, DWORD errorCode, DWORD serviceSpecific)
{
    char header[512];
    FormatActionString(info.FailureTextRes, displayName, header, ARRAYSIZE(header));

    char errorText[512];
    FormatSystemErrorString(errorCode, errorText, ARRAYSIZE(errorText));

    char errorLine[512];
    if (errorText[0] != 0)
    {
        const char* fmt = LoadStr(IDS_SERVICE_ERROR_CODE_FMT);
        if (fmt != NULL)
            _snprintf(errorLine, ARRAYSIZE(errorLine), fmt, errorCode, errorText);
        else
            _snprintf(errorLine, ARRAYSIZE(errorLine), "Error %lu: %s", errorCode, errorText);
    }
    else
    {
        _snprintf(errorLine, ARRAYSIZE(errorLine), "Error %lu.", errorCode);
    }
    errorLine[ARRAYSIZE(errorLine) - 1] = 0;

    char specificLine[512];
    specificLine[0] = 0;
    if (errorCode == ERROR_SERVICE_SPECIFIC_ERROR && serviceSpecific != 0)
    {
        char specificText[512];
        FormatSystemErrorString(serviceSpecific, specificText, ARRAYSIZE(specificText));
        if (specificText[0] != 0)
        {
            const char* fmt = LoadStr(IDS_SERVICE_SPECIFIC_ERROR_FMT);
            if (fmt != NULL)
                _snprintf(specificLine, ARRAYSIZE(specificLine), fmt, serviceSpecific, specificText);
            else
                _snprintf(specificLine, ARRAYSIZE(specificLine), "Service-specific error %lu: %s", serviceSpecific, specificText);
        }
        else
        {
            _snprintf(specificLine, ARRAYSIZE(specificLine), "Service-specific error %lu.", serviceSpecific);
        }
        specificLine[ARRAYSIZE(specificLine) - 1] = 0;
    }

    char message[1024];
    if (specificLine[0] != 0)
        _snprintf(message, ARRAYSIZE(message), "%s\n\n%s\n%s", header, errorLine, specificLine);
    else
        _snprintf(message, ARRAYSIZE(message), "%s\n\n%s", header, errorLine);
    message[ARRAYSIZE(message) - 1] = 0;

    HWND owner = parent != NULL ? parent : SalamanderGeneral->GetMsgBoxParent();
    SalamanderGeneral->SalMessageBox(owner, message, VERSINFO_PLUGINNAME, MB_OK | MB_ICONWARNING);
}

const ServiceActionInfo* FindServiceAction(ServiceActionKind action)
{
    for (size_t i = 0; i < ARRAYSIZE(ServiceActions); ++i)
    {
        if (ServiceActions[i].Action == action)
            return &ServiceActions[i];
    }
    return NULL;
}

BOOL PerformSingleServiceActionCore(const char* serviceName, const char* displayName, const ServiceActionInfo& info,
                                    BOOL silentIfSatisfied, ServiceActionOutcome& outcome)
{
    outcome.Info = &info;
    outcome.ErrorCode = ERROR_SUCCESS;
    outcome.ServiceSpecific = 0;
    outcome.ShowAlreadyMessage = FALSE;

    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (scm == NULL)
    {
        outcome.ErrorCode = GetLastError();
        return FALSE;
    }

    SC_HANDLE service = OpenServiceA(scm, serviceName, info.AccessMask | SERVICE_QUERY_STATUS);
    if (service == NULL)
    {
        outcome.ErrorCode = GetLastError();
        CloseServiceHandle(scm);
        return FALSE;
    }

    SERVICE_STATUS_PROCESS status;
    if (!QueryStatus(service, status))
    {
        outcome.ErrorCode = GetLastError();
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return FALSE;
    }

    if (status.dwCurrentState == info.DesiredState)
    {
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        if (!silentIfSatisfied && info.AlreadyTextRes != 0)
            outcome.ShowAlreadyMessage = TRUE;
        return TRUE;
    }

    DWORD operationError = ERROR_SUCCESS;
    DWORD serviceSpecific = 0;

    if (info.Action == ServiceActionStart)
    {
        if (!StartService(service, 0, NULL))
        {
            operationError = GetLastError();
            if (operationError == ERROR_SERVICE_ALREADY_RUNNING)
                operationError = ERROR_SUCCESS;
        }
    }
    else
    {
        SERVICE_STATUS dummy;
        if (!ControlService(service, info.ControlCode, &dummy))
        {
            operationError = GetLastError();
            if (info.Action == ServiceActionStop && operationError == ERROR_SERVICE_NOT_ACTIVE)
                operationError = ERROR_SUCCESS;
        }
    }

    if (operationError == ERROR_SUCCESS)
        operationError = WaitForServiceState(service, info, serviceSpecific);

    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    outcome.ErrorCode = operationError;
    outcome.ServiceSpecific = serviceSpecific;

    return operationError == ERROR_SUCCESS;
}

BOOL ExecuteServiceActionSequence(ServiceActionWorkerContext& ctx)
{
    const char* serviceName = ctx.ServiceName.c_str();
    const char* friendlyName = ctx.DisplayName.c_str();

    if (ctx.Action == ServiceActionRestart)
    {
        const ServiceActionInfo* stopInfo = FindServiceAction(ServiceActionStop);
        const ServiceActionInfo* startInfo = FindServiceAction(ServiceActionStart);
        if (stopInfo == NULL || startInfo == NULL)
            return FALSE;

        ServiceActionOutcome stopOutcome;
        if (!PerformSingleServiceActionCore(serviceName, friendlyName, *stopInfo, TRUE, stopOutcome))
        {
            ctx.Outcome = stopOutcome;
            return FALSE;
        }

        ServiceActionOutcome startOutcome;
        if (!PerformSingleServiceActionCore(serviceName, friendlyName, *startInfo, TRUE, startOutcome))
        {
            ctx.Outcome = startOutcome;
            return FALSE;
        }

        ctx.Outcome = startOutcome;
        return TRUE;
    }

    const ServiceActionInfo* info = FindServiceAction(ctx.Action);
    if (info == NULL)
        return FALSE;

    ServiceActionOutcome outcome;
    BOOL success = PerformSingleServiceActionCore(serviceName, friendlyName, *info, FALSE, outcome);
    ctx.Outcome = outcome;
    return success;
}

unsigned __stdcall ServiceActionThreadProc(void* param)
{
    ServiceActionWorkerContext* ctx = reinterpret_cast<ServiceActionWorkerContext*>(param);
    if (ctx != NULL)
    {
        ctx->Success = ExecuteServiceActionSequence(*ctx);
        if (ctx->CompletionEvent != NULL)
            SetEvent(ctx->CompletionEvent);
    }
    return 0;
}

} // namespace

BOOL RunServiceAction(HWND parent, const char* serviceName, const char* displayName, ServiceActionKind action)
{
    if (serviceName == NULL || serviceName[0] == 0)
        return FALSE;

    const char* friendlyName = (displayName != NULL && displayName[0] != 0) ? displayName : serviceName;

    ServiceActionWorkerContext context(serviceName, friendlyName, action);
    if (context.CompletionEvent == NULL)
        return FALSE;

    HANDLE threadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, ServiceActionThreadProc, &context, 0, NULL));
    if (threadHandle == NULL)
    {
        context.Success = ExecuteServiceActionSequence(context);
        if (context.CompletionEvent != NULL)
            SetEvent(context.CompletionEvent);
    }
    else
    {
        if (WaitForSingleObject(context.CompletionEvent, 0) != WAIT_OBJECT_0)
        {
            char progressText[512];
            progressText[0] = 0;

            std::wstring captionW = AnsiToWide(LoadStr(IDS_SERVICE_PROGRESS_CAPTION));
            std::wstring messageW;

            if (action == ServiceActionRestart)
            {
                const char* restartFmt = LoadStr(IDS_SERVICE_PROGRESS_RESTARTING);
                if (restartFmt == NULL)
                    restartFmt = "Restarting '%s'...";
                _snprintf(progressText, ARRAYSIZE(progressText), restartFmt, friendlyName);
                progressText[ARRAYSIZE(progressText) - 1] = 0;
                messageW = AnsiToWide(progressText);
            }
            else
            {
                const ServiceActionInfo* info = FindServiceAction(action);
                if (info != NULL)
                {
                    FormatActionString(info->ProgressTextRes, friendlyName, progressText, ARRAYSIZE(progressText));
                    messageW = AnsiToWide(progressText);
                }
            }

            if (messageW.empty())
                messageW = AnsiToWide(friendlyName);

            PFN_TaskDialogIndirect taskDialog = ResolveTaskDialog();
            if (taskDialog != NULL)
            {
                TaskDialogContext dialogContext(context.CompletionEvent);
                TASKDIALOGCONFIG config = {};
                config.cbSize = sizeof(config);
                config.hwndParent = parent;
                config.dwFlags = TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CALLBACK_TIMER | TDF_POSITION_RELATIVE_TO_WINDOW;
                config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
                config.pszWindowTitle = captionW.empty() ? NULL : captionW.c_str();
                config.pszContent = messageW.c_str();
                config.pfCallback = ServiceActionTaskDialogCallback;
                config.lpCallbackData = reinterpret_cast<LONG_PTR>(&dialogContext);
                taskDialog(&config, NULL, NULL, NULL);
            }
            else
            {
                PumpMessageLoopUntil(context.CompletionEvent);
            }
        }

        WaitForSingleObject(threadHandle, INFINITE);
        CloseHandle(threadHandle);
    }

    if (context.CompletionEvent != NULL)
        WaitForSingleObject(context.CompletionEvent, INFINITE);

    const ServiceActionInfo* info = context.Outcome.Info;
    if (!context.Success)
    {
        if (info != NULL)
            ShowServiceOperationError(parent, friendlyName, *info, context.Outcome.ErrorCode, context.Outcome.ServiceSpecific);
        return FALSE;
    }

    if (context.Outcome.ShowAlreadyMessage && info != NULL && info->AlreadyTextRes != 0)
    {
        char text[512];
        FormatActionString(info->AlreadyTextRes, friendlyName, text, ARRAYSIZE(text));
        SalamanderGeneral->SalMessageBox(parent != NULL ? parent : SalamanderGeneral->GetMsgBoxParent(), text,
                                         VERSINFO_PLUGINNAME, MB_OK | MB_ICONINFORMATION);
    }

    return TRUE;
}

//---------------------------------------------------------------------------
// LASTERRORMESSAGE
//---------------------------------------------------------------------------
const char *GetLastErrorMessage()
{
  HLOCAL hlocal = NULL;
  bool bOK=FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                         NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         (LPTSTR) &hlocal, 0, NULL);
  if (bOK!=0)
  {
         // Netzwerkfehler ?
         HMODULE hDll = LoadLibraryEx(TEXT("netmsg.dll"), NULL, DONT_RESOLVE_DLL_REFERENCES);
         if (hDll != NULL)
         {
            FormatMessage(
               FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM,
               hDll, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
               (PTSTR) &hlocal, 0, NULL);
            FreeLibrary(hDll);
        }
  }
	char * message = (char*)hlocal+'\0';
  if (hlocal != NULL)
    return ((char *)hlocal)+'\0';
  else
    return NULL;
}

DWORD GetServiceStatus(char *cSvcName)
{
  SC_HANDLE schService, schSCManager;
  SERVICE_STATUS ssStatus;
	memset( &ssStatus, 0, sizeof(SERVICE_STATUS) );

  DWORD dwOldCheckPoint=0;
  DWORD dwStartTickCount=0;
  DWORD dwWaitTime=0;
  DWORD dwStatus=0;
  DWORD dwReturn=0;

  //SC_MANAGER_ENUMERATE_SERVICE
  schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
  if(schSCManager==NULL)
		return GetLastError();
  schService = OpenService( schSCManager, cSvcName, SERVICE_CONTROL_INTERROGATE);
  if (schService == NULL)
		return GetLastError();


  dwReturn = ControlService( schService, SERVICE_CONTROL_INTERROGATE, &ssStatus);
  if (!QueryServiceStatus(schService,&ssStatus) )
			return GetLastError();

	//GetLastErrorMessage();

  dwStartTickCount = GetTickCount();
  dwOldCheckPoint = ssStatus.dwCheckPoint;
  
  while (ssStatus.dwCurrentState == SERVICE_START_PENDING ||
         ssStatus.dwCurrentState == SERVICE_STOP_PENDING  ||
         ssStatus.dwCurrentState == SERVICE_CONTINUE_PENDING ||
         ssStatus.dwCurrentState == SERVICE_PAUSE_PENDING)
  {
      dwWaitTime = ssStatus.dwWaitHint / 10;
      if( dwWaitTime < 1000 )
          dwWaitTime = 1000;
      else if ( dwWaitTime > 10000 )
          dwWaitTime = 10000;
      Sleep( dwWaitTime );
      if (!QueryServiceStatus(schService,&ssStatus) )break;
      if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
      {
          dwStartTickCount = GetTickCount();
          dwOldCheckPoint = ssStatus.dwCheckPoint;
      }
      else
      {
        if(GetTickCount()- dwStartTickCount > ssStatus.dwWaitHint)
          break;
      }
  }
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
  return ssStatus.dwCurrentState;
}


DWORD SStartService(char *cSvcName)
{
    // L@ MSDN
    SERVICE_STATUS ssStatus;
    DWORD dwOldCheckPoint;
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwStatus;

    SC_HANDLE schSCManager, schService;

    schSCManager = OpenSCManager(NULL,NULL,SERVICE_START|SERVICE_QUERY_STATUS);
	if (schSCManager == NULL)
		return GetLastError();

    schService = OpenService(schSCManager,cSvcName,SERVICE_START|SERVICE_QUERY_STATUS);
    if (schService == NULL)
			return GetLastError();
		if (!QueryServiceStatus(schService,&ssStatus) )
                    OutputDebugString("QueryServiceStatus failed.");

    if (!StartService(schService,0,NULL) )
                    OutputDebugString("servicemanager-tStartService");
    else
                    OutputDebugString("servicemanager tStartServicepending.");
    if (!QueryServiceStatus(schService,&ssStatus) )
                    OutputDebugString("QueryServiceStatus failed.");
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;
    while (ssStatus.dwCurrentState == SERVICE_START_PENDING || SERVICE_STOP_PENDING)
   {
        dwWaitTime = ssStatus.dwWaitHint / 10;
        if( dwWaitTime < 1000 )
            dwWaitTime = 1000;
        else if ( dwWaitTime > 10000 )
            dwWaitTime = 10000;
        Sleep( dwWaitTime );
        if (!QueryServiceStatus(schService,&ssStatus) )break;
        if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
        {
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
          if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
            break;
        }
    }
    if (ssStatus.dwCurrentState == SERVICE_RUNNING)
        dwStatus = NO_ERROR;
    else
        dwStatus = GetLastError();
    CloseServiceHandle(schService); 
    return dwStatus;
}

DWORD SetStatus (char *cSvcName, SControlType sct)
{
  SC_HANDLE schService, schSCManager;
  SERVICE_STATUS ssStatus;
  DWORD dwReturn;
	schService = NULL;

	switch (sct)
	{
		case Stop:
						schService = OpenSCManager( NULL, NULL, SERVICE_STOP);break;
		case Pause:
						schService = OpenSCManager( NULL, NULL, SERVICE_CONTROL_PAUSE);break;
		case Continue:
						schService = OpenSCManager( NULL, NULL, SERVICE_CONTROL_CONTINUE);break;
		case Interrogate:
						schService = OpenSCManager( NULL, NULL, SERVICE_CONTROL_INTERROGATE);break;
		case Shutdown:
						schService = OpenSCManager( NULL, NULL, SERVICE_CONTROL_SHUTDOWN);;break;
	}
  schSCManager = OpenSCManager( NULL, NULL, SERVICE_STOP);
  if (schSCManager==NULL)
	  return GetLastError();
  switch (sct)
  {
    case Stop:
            schService = OpenService( schSCManager, cSvcName, SERVICE_STOP);break;
    case Pause:
            schService = OpenService( schSCManager, cSvcName, SERVICE_CONTROL_PAUSE);break;
    case Continue:
            schService = OpenService( schSCManager, cSvcName, SERVICE_CONTROL_CONTINUE);break;
    case Interrogate:
            schService = OpenService( schSCManager, cSvcName, SERVICE_CONTROL_INTERROGATE);break;
    case Shutdown:
            schService = OpenService( schSCManager, cSvcName, SERVICE_CONTROL_SHUTDOWN);break;
  }
  if (schService == NULL)
	  return GetLastError();
  else
  {
    switch (sct)
    {
      case Stop:
              dwReturn = ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus);break;
      case Pause:
              dwReturn = ControlService( schService, SERVICE_CONTROL_PAUSE, &ssStatus);break;
      case Continue:
              dwReturn = ControlService( schService, SERVICE_CONTROL_CONTINUE, &ssStatus);break;
      case Interrogate:
              dwReturn = ControlService( schService, SERVICE_CONTROL_INTERROGATE, &ssStatus);break;
      case Shutdown:
              dwReturn = ControlService( schService, SERVICE_CONTROL_SHUTDOWN, &ssStatus);break;
    }
    if (dwReturn)
      return NO_ERROR;
  }
  return GetLastError();
}
QUERY_SERVICE_CONFIG *GetQueryServiceConfig(char *szService)
{  
	QUERY_SERVICE_CONFIG* g_psc = NULL;
	//ToDO: Select Computer
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SERVICE_QUERY_CONFIG);
	SC_HANDLE hService = OpenService(hSCM, szService, SERVICE_QUERY_CONFIG);

	//if (hSCM==ERROR_ACCESS_DENIED)//Insufficient rights
	//	return ERROR_ACCESS_DENIED; 

	QUERY_SERVICE_CONFIG sc;
	DWORD dwBytesNeeded = 0;

	// Try to get information about the query
	BOOL bRetVal = QueryServiceConfig(hService, &sc, sizeof(QUERY_SERVICE_CONFIG),&dwBytesNeeded);

	if (!bRetVal)
	{
		DWORD retVal = GetLastError();

		// buffer size is small. 
		// Required size is in dwBytesNeeded
		if (ERROR_INSUFFICIENT_BUFFER == retVal) 
		{
			DWORD dwBytes = sizeof(QUERY_SERVICE_CONFIG) + dwBytesNeeded;
			g_psc = new QUERY_SERVICE_CONFIG[dwBytesNeeded];
			bRetVal = QueryServiceConfig(hService, g_psc, dwBytes, &dwBytesNeeded);
			if (!bRetVal) 
			{
				GetLastErrorMessage();
				delete [] g_psc;
				g_psc = NULL;
			}
		}
		else	//insufficient rights
		{
			retVal = GetLastError();
			switch (retVal)
			{
				case ERROR_ACCESS_DENIED:
					break;
				case ERROR_INSUFFICIENT_BUFFER:
					break;
				case ERROR_INVALID_HANDLE:
					break;
			}
			return NULL;
		}
	}
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	return g_psc;
}

// Get Status (Fast)
DWORD SGetStatus(char *cSvcName)
{
  SC_HANDLE schService, schSCManager;
  SERVICE_STATUS ssStatus;

  DWORD dwOldCheckPoint;
  DWORD dwStartTickCount;
  DWORD dwWaitTime;
  //DWORD dwStatus;
  DWORD dwReturn;

  schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS);
  schService = OpenService( schSCManager, cSvcName, SERVICE_CONTROL_INTERROGATE);
  dwReturn = ControlService( schService, SERVICE_CONTROL_INTERROGATE, &ssStatus);
  QueryServiceStatus(schService,&ssStatus);

  dwStartTickCount = GetTickCount();
  dwOldCheckPoint = ssStatus.dwCheckPoint;
  while (ssStatus.dwCurrentState == SERVICE_START_PENDING ||
         ssStatus.dwCurrentState == SERVICE_STOP_PENDING  ||
         ssStatus.dwCurrentState == SERVICE_CONTINUE_PENDING ||
         ssStatus.dwCurrentState == SERVICE_PAUSE_PENDING)
  {
      dwWaitTime = ssStatus.dwWaitHint / 10;
      if( dwWaitTime < 1000 )
          dwWaitTime = 1000;
      else if ( dwWaitTime > 10000 )
          dwWaitTime = 10000;
      Sleep( dwWaitTime );
      if (!QueryServiceStatus(schService,&ssStatus) )break;
      if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
      {
          dwStartTickCount = GetTickCount();
          dwOldCheckPoint = ssStatus.dwCheckPoint;
      }
      else
      {
        if(GetTickCount()- dwStartTickCount > ssStatus.dwWaitHint)
          break;
      }
  }
  return ssStatus.dwCurrentState;
}
DWORD DoQuerySvc(char *szSvcName,char *szDescription, char *szDependencies)
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    LPQUERY_SERVICE_CONFIG lpsc; 
    LPSERVICE_DESCRIPTION lpsd=NULL;
    DWORD dwBytesNeeded, cbBufSize, dwError; 

    // Get a handle to the SCM database. 
    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
 
    if (schSCManager==NULL) 
        return NULL;

		// Get a handle to the service.
    schService = OpenService( 
        schSCManager,          // SCM database 
        szSvcName,             // name of service 
        SERVICE_QUERY_CONFIG); // need query config access 
 
    if (schService == NULL)
    { 
        CloseServiceHandle(schSCManager);
        return NULL;
    }

    // Get the configuration information.
 
    if( !QueryServiceConfig(schService,NULL,0, &dwBytesNeeded))
    {
        dwError = GetLastError();
        if( ERROR_INSUFFICIENT_BUFFER == dwError )
        {
            cbBufSize = dwBytesNeeded;
            lpsc = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LMEM_FIXED, cbBufSize);
        }
        else
        {
            printf("QueryServiceConfig failed (%d)", dwError);
            goto cleanup; 
        }
    }
  
    if( !QueryServiceConfig(schService, lpsc, cbBufSize, &dwBytesNeeded) ) 
    {
        printf("QueryServiceConfig failed (%d)", GetLastError());
        goto cleanup;
    }

    if( !QueryServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION,NULL, 0, &dwBytesNeeded))
    {
        dwError = GetLastError();
        if( ERROR_INSUFFICIENT_BUFFER == dwError )
        {
            cbBufSize = dwBytesNeeded;
            lpsd = (LPSERVICE_DESCRIPTION) LocalAlloc(LMEM_FIXED, cbBufSize);
        }
        else
        {
            goto cleanup; 
        }
    }
 
    if (! QueryServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION,(LPBYTE) lpsd, cbBufSize, &dwBytesNeeded) ) 
    {
        printf("QueryServiceConfig2 failed (%d)", GetLastError());
        goto cleanup;
    }
 
    // Print the configuration information.

		//char buf[1000];
		//buf[999] = 0;
	 // _snprintf(buf, 1000,"%s", lpsd->lpDescription);

	
 
    //_tprintf(TEXT("%s configuration: \n"), szSvcName);
    //_tprintf(TEXT("  Type: 0x%x\n"), lpsc->dwServiceType);
    //_tprintf(TEXT("  Start Type: 0x%x\n"), lpsc->dwStartType);
    //_tprintf(TEXT("  Error Control: 0x%x\n"), lpsc->dwErrorControl);
    //_tprintf(TEXT("  Binary path: %s\n"), lpsc->lpBinaryPathName);
    //_tprintf(TEXT("  Account: %s\n"), lpsc->lpServiceStartName);

    //if (lpsd->lpDescription != NULL && lstrcmp(lpsd->lpDescription, TEXT("")) != 0)
    //    _tprintf(TEXT("  Description: %s\n"), lpsd->lpDescription);
    //if (lpsc->lpLoadOrderGroup != NULL && lstrcmp(lpsc->lpLoadOrderGroup, TEXT("")) != 0)
    //    _tprintf(TEXT("  Load order group: %s\n"), lpsc->lpLoadOrderGroup);
    //if (lpsc->dwTagId != 0)
    //    _tprintf(TEXT("  Tag ID: %d\n"), lpsc->dwTagId);
    //if (lpsc->lpDependencies != NULL && lstrcmp(lpsc->lpDependencies, TEXT("")) != 0)
    //    _tprintf(TEXT("  Dependencies: %s\n"), lpsc->lpDependencies);
		//char description[1000];
		//_snprintf(description,1000,"%s",lpsd->lpDescription);
		//strncpy(description,lpsd->lpDescription);

	if (lpsd->lpDescription != NULL && lstrcmp(lpsd->lpDescription, TEXT("")) != 0)
	{
		strcpy(szDescription,lpsd->lpDescription);
	}
    LocalFree(lpsc); 
    LocalFree(lpsd);

cleanup:
    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
	return NULL;
}





DWORD ChangeSvc(char *szSvcName,SvcCommandType eCommandType, SvcCommand eCommand)
{
		SC_HANDLE schSCManager;SC_HANDLE schService;DWORD returnvalue=0;

		// Get a handle to the SCM database. 
    schSCManager = OpenSCManager( 
    NULL,                    // local computer
    NULL,                    // ServicesActive database 
    SC_MANAGER_ALL_ACCESS);  // full access rights 

		if (NULL == schSCManager) 
        return GetLastError();

		// Get a handle to the service.
    schService = OpenService( 
        schSCManager,            // SCM database 
        szSvcName,               // name of service 
        SERVICE_CHANGE_CONFIG);  // need change config access 

		if (schService == NULL)
    { 
        CloseServiceHandle(schSCManager);
        return GetLastError();
    }
    // Change the service start type.
		if (eCommandType==SVC_CHANGE_STARTTYPE)
		{
			int startuptype=SERVICE_NO_CHANGE;
			switch(eCommand)
			{
				case SVC_STARTTYPE_ONDEMAND:
					startuptype=SERVICE_DEMAND_START;
					break;
				case SVC_STARTTYPE_AUTO:
					startuptype=SERVICE_AUTO_START;
					break;
				case SVC_STARTTYPE_DISABLED:
					startuptype=SERVICE_DISABLED;
					break;
			}
			if (! ChangeServiceConfig( 
					schService,            // handle of service 
					SERVICE_NO_CHANGE,     // service type: no change 
					startuptype,					 // service start type 
					SERVICE_NO_CHANGE,     // error control: no change 
					NULL,                  // binary path: no change 
					NULL,                  // load order group: no change 
					NULL,                  // tag ID: no change 
					NULL,                  // dependencies: no change 
					NULL,                  // account name: no change 
					NULL,                  // password: no change 
					NULL) )                // display name: no change
			{
					returnvalue =  GetLastError(); 
			}
			else returnvalue = NULL;
		}
		CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
		return returnvalue;
}







DWORD DoDeleteSvc(char *szSvcName)
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
		DWORD returnvalue=0;
    // Get a handle to the SCM database.
 
    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
 
		if (NULL == schSCManager) 
        return GetLastError();


    // Get a handle to the service.

    schService = OpenService( 
        schSCManager,       // SCM database 
        szSvcName,          // name of service 
        DELETE);            // need delete access 
 
		if (schService == NULL)
    { 
        CloseServiceHandle(schSCManager);
        return GetLastError();
    }

    // Delete the service.
 
    if (! DeleteService(schService) ) 
    {
        returnvalue =  GetLastError(); 
    }
    else returnvalue = NULL;
 
    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
		return returnvalue;
}