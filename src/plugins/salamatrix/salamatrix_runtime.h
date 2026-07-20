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
            HostRegistered &= General->RegisterService(SALAMATRIX_SERVICE_UI, SALAMATRIX_UI_VERSION_1_0, &UIService, "Salamatrix Framework");
            HostRegistered &= General->RegisterService(SALAMATRIX_SERVICE_COMMANDS, SALAMATRIX_COMMANDS_VERSION_1_0, &CommandService, "Salamatrix Framework");
            HostRegistered &= General->RegisterService(SALAMATRIX_SERVICE_FILEOPERATIONS, SALAMATRIX_FILEOPERATIONS_VERSION_1_0, &FileOperationsService, "Salamatrix Framework");
            HostRegistered &= General->RegisterService(SALAMATRIX_SERVICE_AUTOMATION_ADAPTER, SALAMATRIX_AUTOMATION_VERSION_1_0, &ScriptRoot, "Salamatrix Framework");
        }
    }

    ~RuntimeServices()
    {
        if (General != NULL && HostRegistered)
        {
            General->UnregisterService(SALAMATRIX_SERVICE_AUTOMATION_ADAPTER, &ScriptRoot);
            General->UnregisterService(SALAMATRIX_SERVICE_FILEOPERATIONS, &FileOperationsService);
            General->UnregisterService(SALAMATRIX_SERVICE_COMMANDS, &CommandService);
            General->UnregisterService(SALAMATRIX_SERVICE_UI, &UIService);
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
