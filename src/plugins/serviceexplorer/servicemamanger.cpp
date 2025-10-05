#include "precomp.h"
#include "servicemanager.h"

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
			OutputDebugStr("QueryServiceStatus failed.");

    if (!StartService(schService,0,NULL) )
			OutputDebugStr("servicemanager-tStartService");
    else
			OutputDebugStr("servicemanager tStartServicepending.");
    if (!QueryServiceStatus(schService,&ssStatus) )
			OutputDebugStr("QueryServiceStatus failed.");
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
    SERVICE_STATUS ssStatus; 

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