// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_runtime.h
    Minimal in-process service registry and runtime service wiring.
*/

#pragma once

#include "salamatrix_automation.h"

namespace Salamatrix
{
namespace Runtime
{

typedef BOOL(WINAPI* FSalamanderRegisterService)(const char* serviceId, DWORD version, void* serviceInterface, const char* providerName);
typedef BOOL(WINAPI* FSalamanderUnregisterService)(const char* serviceId, void* serviceInterface);
typedef BOOL(WINAPI* FSalamanderQueryService)(const char* serviceId, DWORD minimumVersion, void** serviceInterface, DWORD* providedVersion, const char** providerName);

static BOOL WINAPI RegisterHostService(const char* serviceId, DWORD version, void* serviceInterface, const char* providerName)
{
    HMODULE host = GetModuleHandle(NULL);
    if (host == NULL)
        return FALSE;

    FSalamanderRegisterService registerService =
        (FSalamanderRegisterService)GetProcAddress(host, "SalamanderRegisterService");
    if (registerService == NULL)
        return FALSE;

    return registerService(serviceId, version, serviceInterface, providerName);
}

static BOOL WINAPI UnregisterHostService(const char* serviceId, void* serviceInterface)
{
    HMODULE host = GetModuleHandle(NULL);
    if (host == NULL)
        return FALSE;

    FSalamanderUnregisterService unregisterService =
        (FSalamanderUnregisterService)GetProcAddress(host, "SalamanderUnregisterService");
    if (unregisterService == NULL)
        return FALSE;

    return unregisterService(serviceId, serviceInterface);
}

static void* WINAPI QueryHostService(const char* serviceId, DWORD minimumVersion, DWORD* providedVersion)
{
    HMODULE host = GetModuleHandle(NULL);
    if (host == NULL)
        return NULL;

    FSalamanderQueryService queryService =
        (FSalamanderQueryService)GetProcAddress(host, "SalamanderQueryService");
    if (queryService == NULL)
        return NULL;

    void* serviceInterface = NULL;
    DWORD actualVersion = 0;
    if (!queryService(serviceId, minimumVersion, &serviceInterface, &actualVersion, NULL))
        return NULL;

    if (providedVersion != NULL)
        *providedVersion = actualVersion;

    return serviceInterface;
}

#define SALAMATRIX_SERVICE_RUNTIME "Salamatrix.Runtime"
#define SALAMATRIX_RUNTIME_VERSION_1_0 0x00010000

struct ServiceQuery
{
    const char* ServiceId;
    DWORD MinimumVersion;
    DWORD Flags;

    ServiceQuery()
        : ServiceId(NULL),
          MinimumVersion(0),
          Flags(0)
    {
    }
};

struct ServiceResult
{
    void* Interface;
    DWORD Version;
    const char* ProviderName;

    ServiceResult()
        : Interface(NULL),
          Version(0),
          ProviderName(NULL)
    {
    }
};

class ServiceRegistry
{
private:
    enum
    {
        MaxServices = 16
    };

    struct ServiceRecord
    {
        const char* ServiceId;
        DWORD Version;
        void* Interface;
        const char* ProviderName;

        ServiceRecord()
            : ServiceId(NULL),
              Version(0),
              Interface(NULL),
              ProviderName(NULL)
        {
        }
    };

    ServiceRecord Services[MaxServices];
    int Count;

    static BOOL WINAPI SameServiceId(const char* left, const char* right)
    {
        if (left == NULL || right == NULL)
            return FALSE;
        return strcmp(left, right) == 0;
    }

public:
    ServiceRegistry()
        : Count(0)
    {
    }

    BOOL WINAPI RegisterService(const char* serviceId, DWORD version, void* serviceInterface, const char* providerName)
    {
        if (serviceId == NULL || serviceInterface == NULL || version == 0)
            return FALSE;

        for (int i = 0; i < Count; ++i)
        {
            if (SameServiceId(Services[i].ServiceId, serviceId))
                return FALSE;
        }

        if (Count >= MaxServices)
            return FALSE;

        Services[Count].ServiceId = serviceId;
        Services[Count].Version = version;
        Services[Count].Interface = serviceInterface;
        Services[Count].ProviderName = providerName;
        ++Count;
        return TRUE;
    }

    BOOL WINAPI QueryService(const ServiceQuery& query, ServiceResult* result) const
    {
        if (result != NULL)
            *result = ServiceResult();
        if (query.ServiceId == NULL)
            return FALSE;

        for (int i = 0; i < Count; ++i)
        {
            if (SameServiceId(Services[i].ServiceId, query.ServiceId) && Services[i].Version >= query.MinimumVersion)
            {
                if (result != NULL)
                {
                    result->Interface = Services[i].Interface;
                    result->Version = Services[i].Version;
                    result->ProviderName = Services[i].ProviderName;
                }
                return TRUE;
            }
        }

        return FALSE;
    }

    void* WINAPI QueryService(const char* serviceId, DWORD minimumVersion, DWORD* providedVersion) const
    {
        ServiceQuery query;
        query.ServiceId = serviceId;
        query.MinimumVersion = minimumVersion;

        ServiceResult result;
        if (!QueryService(query, &result))
            return NULL;

        if (providedVersion != NULL)
            *providedVersion = result.Version;
        return result.Interface;
    }

    int WINAPI GetCount() const
    {
        return Count;
    }
};

class LocalUIService : public UI::IUIService
{
public:
    virtual DWORD WINAPI GetVersion() const
    {
        return SALAMATRIX_UI_VERSION_1_0;
    }

    virtual UI::IProgressDialog* WINAPI CreateProgressDialog(CSalamanderForOperationsAbstract* operations)
    {
        if (operations == NULL)
            return NULL;
        return new UI::ProgressDialog(operations);
    }

    virtual void WINAPI DestroyProgressDialog(UI::IProgressDialog* dialog)
    {
        delete static_cast<UI::ProgressDialog*>(dialog);
    }
};

class RuntimeServices
{
private:
    LocalUIService UIService;
    Commands::CommandService CommandService;
    FileOperations::FileOperationsService FileOperationsService;
    Automation::ScriptRootAdapter ScriptRoot;
    ServiceRegistry Registry;
    CSalamanderGeneralAbstract* General;
    BOOL Registered;
    BOOL HostRegistered;

    RuntimeServices(const RuntimeServices&);
    RuntimeServices& operator=(const RuntimeServices&);

public:
    explicit RuntimeServices(CSalamanderGeneralAbstract* general, BOOL registerHostServices = TRUE)
        : UIService(),
          CommandService(general),
          FileOperationsService(&CommandService),
          ScriptRoot(&UIService, &CommandService, &FileOperationsService),
          Registry(),
          General(general),
          Registered(FALSE),
          HostRegistered(FALSE)
    {
        Registered = TRUE;
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_UI, SALAMATRIX_UI_VERSION_1_0, &UIService, "Salamatrix Framework");
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_COMMANDS, SALAMATRIX_COMMANDS_VERSION_1_0, &CommandService, "Salamatrix Framework");
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_FILEOPERATIONS, SALAMATRIX_FILEOPERATIONS_VERSION_1_0, &FileOperationsService, "Salamatrix Framework");
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_AUTOMATION_ADAPTER, SALAMATRIX_AUTOMATION_VERSION_1_0, &ScriptRoot, "Salamatrix Framework");

        if (General != NULL && registerHostServices)
        {
            HostRegistered = TRUE;
            HostRegistered &= RegisterHostService(SALAMATRIX_SERVICE_UI, SALAMATRIX_UI_VERSION_1_0, &UIService, "Salamatrix Framework");
            HostRegistered &= RegisterHostService(SALAMATRIX_SERVICE_COMMANDS, SALAMATRIX_COMMANDS_VERSION_1_0, &CommandService, "Salamatrix Framework");
            HostRegistered &= RegisterHostService(SALAMATRIX_SERVICE_FILEOPERATIONS, SALAMATRIX_FILEOPERATIONS_VERSION_1_0, &FileOperationsService, "Salamatrix Framework");
            HostRegistered &= RegisterHostService(SALAMATRIX_SERVICE_AUTOMATION_ADAPTER, SALAMATRIX_AUTOMATION_VERSION_1_0, &ScriptRoot, "Salamatrix Framework");
        }
    }

    ~RuntimeServices()
    {
        if (General != NULL && HostRegistered)
        {
            UnregisterHostService(SALAMATRIX_SERVICE_AUTOMATION_ADAPTER, &ScriptRoot);
            UnregisterHostService(SALAMATRIX_SERVICE_FILEOPERATIONS, &FileOperationsService);
            UnregisterHostService(SALAMATRIX_SERVICE_COMMANDS, &CommandService);
            UnregisterHostService(SALAMATRIX_SERVICE_UI, &UIService);
        }
    }

    BOOL WINAPI IsRegistered() const
    {
        return Registered;
    }

    BOOL WINAPI IsHostRegistered() const
    {
        return HostRegistered;
    }

    ServiceRegistry* WINAPI Services()
    {
        return &Registry;
    }

    UI::IUIService* WINAPI UI()
    {
        return &UIService;
    }

    Commands::ICommandService* WINAPI Commands()
    {
        return &CommandService;
    }

    FileOperations::IFileOperationsService* WINAPI FileOperations()
    {
        return &FileOperationsService;
    }

    Automation::ScriptRootAdapter* WINAPI Script()
    {
        return &ScriptRoot;
    }
};

} // namespace Runtime
} // namespace Salamatrix
