// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_runtime.h
    Minimal in-process service registry and runtime service wiring.
*/

#pragma once

#include <vector>

#include "salamatrix_automation.h"
#include "salamatrix_ai.h"
#include "salamatrix_events.h"
#include "salamatrix_extensions.h"
#include "salamatrix_runtime_api.h"
#include "salamatrix_sides.h"
#include "salamatrix_storage.h"

namespace Salamatrix
{
namespace Runtime
{

static int ShowUtf8MessageBox(
    HWND parent,
    const char* message,
    const char* title,
    UINT flags)
{
    const char* safeMessage = message != NULL ? message : "";
    const char* safeTitle = title != NULL ? title : "Salamander";
    int messageLength = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, safeMessage, -1, NULL, 0);
    int titleLength = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, safeTitle, -1, NULL, 0);
    if (messageLength <= 0 || titleLength <= 0)
        return 0;
    std::wstring messageWide(static_cast<size_t>(messageLength), L'\0');
    std::wstring titleWide(static_cast<size_t>(titleLength), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, safeMessage, -1,
            &messageWide[0], messageLength) <= 0 ||
        MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, safeTitle, -1,
            &titleWide[0], titleLength) <= 0)
        return 0;
    return MessageBoxW(
        parent, messageWide.c_str(), titleWide.c_str(),
        flags != 0 ? flags : (MB_OK | MB_ICONINFORMATION));
}

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
private:
    CSalamanderGeneralAbstract* General;

    static BOOL Utf8ToWide(
        const char* value,
        std::wstring& result)
    {
        result.clear();
        const char* safeValue = value != NULL ? value : "";
        int length = MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, safeValue, -1, NULL, 0);
        if (length <= 0)
            return FALSE;
        result.resize(static_cast<size_t>(length));
        return MultiByteToWideChar(
                   CP_UTF8, MB_ERR_INVALID_CHARS, safeValue, -1,
                   &result[0], length) > 0;
    }

    static BOOL WideToUtf8(
        const wchar_t* value,
        char* result,
        DWORD resultCapacity)
    {
        if (value == NULL || result == NULL || resultCapacity == 0)
            return FALSE;
        int length = WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, NULL, 0, NULL, NULL);
        if (length <= 0 || static_cast<DWORD>(length) > resultCapacity)
            return FALSE;
        return WideCharToMultiByte(
                   CP_UTF8, WC_ERR_INVALID_CHARS, value, -1,
                   result, static_cast<int>(resultCapacity), NULL, NULL) > 0;
    }

    static BOOL BuildFilter(
        const char* filter,
        std::vector<wchar_t>& result)
    {
        const char* source = filter != NULL && filter[0] != '\0'
                                 ? filter
                                 : "All files (*.*)|*.*";
        std::wstring wide;
        if (!Utf8ToWide(source, wide))
            return FALSE;
        for (size_t index = 0; index < wide.size(); ++index)
        {
            if (wide[index] == L'|')
                wide[index] = L'\0';
        }
        if (wide.empty() || wide[wide.size() - 1] != L'\0')
            wide.push_back(L'\0');
        wide.push_back(L'\0');
        result.assign(wide.begin(), wide.end());
        return TRUE;
    }

public:
    explicit LocalUIService(CSalamanderGeneralAbstract* general = NULL)
        : General(general)
    {
    }

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

    virtual int WINAPI ShowMessageBox(
        HWND parent,
        const char* message,
        const char* title,
        UINT flags)
    {
        return ShowUtf8MessageBox(parent, message, title, flags);
    }

    virtual BOOL WINAPI CopyTextToClipboard(
        const char* text,
        BOOL showEcho,
        HWND echoParent)
    {
        return General != NULL && text != NULL
                   ? General->CopyTextToClipboard(
                         text, -1, showEcho, echoParent)
                   : FALSE;
    }

    virtual BOOL WINAPI PickFile(
        HWND parent,
        BOOL save,
        const char* title,
        const char* filter,
        const char* initialPath,
        char* result,
        DWORD resultCapacity)
    {
        if (result == NULL || resultCapacity == 0)
            return FALSE;
        result[0] = '\0';

        std::wstring titleWide;
        std::wstring initialWide;
        std::vector<wchar_t> filterWide;
        if (!Utf8ToWide(title != NULL ? title : "Salamander", titleWide) ||
            !Utf8ToWide(initialPath != NULL ? initialPath : "", initialWide) ||
            !BuildFilter(filter, filterWide))
            return FALSE;

        std::vector<wchar_t> path(SAL_MAX_PATH);
        if (!initialWide.empty())
        {
            if (initialWide.size() >= path.size())
                return FALSE;
            memcpy(&path[0], initialWide.c_str(),
                   initialWide.size() * sizeof(wchar_t));
            path[initialWide.size()] = L'\0';
        }

        OPENFILENAMEW dialog;
        memset(&dialog, 0, sizeof(dialog));
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = parent;
        dialog.lpstrFilter = &filterWide[0];
        dialog.lpstrFile = &path[0];
        dialog.nMaxFile = static_cast<DWORD>(path.size());
        dialog.lpstrTitle = titleWide.c_str();
        dialog.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
        if (save)
            dialog.Flags |= OFN_OVERWRITEPROMPT;

        BOOL selected = FALSE;
#ifdef UNICODE
        if (General != NULL)
            selected = save
                           ? General->SafeGetSaveFileName(
                                 reinterpret_cast<LPOPENFILENAME>(&dialog))
                           : General->SafeGetOpenFileName(
                                 reinterpret_cast<LPOPENFILENAME>(&dialog));
        else
#endif
            selected = save ? GetSaveFileNameW(&dialog)
                            : GetOpenFileNameW(&dialog);
        if (!selected)
            return FALSE;
        return WideToUtf8(&path[0], result, resultCapacity);
    }

    virtual UI::IDialog* WINAPI CreateDialog(const UI::DialogOptions& options)
    {
        return new UI::NativeDialog(options);
    }

    virtual void WINAPI DestroyDialog(UI::IDialog* dialog)
    {
        if (dialog != NULL)
            dialog->Release();
    }
};

class RuntimeService : public IRuntimeService
{
private:
    enum
    {
        MaxAdapters = 32
    };

    IRuntimeAdapter* Adapters[MaxAdapters];
    int AdapterCount;

    static BOOL WINAPI SameRuntimeId(const char* left, const char* right)
    {
        if (left == NULL || right == NULL)
            return FALSE;
        return _stricmp(left, right) == 0;
    }

public:
    RuntimeService()
        : AdapterCount(0)
    {
        memset(Adapters, 0, sizeof(Adapters));
    }

    virtual DWORD WINAPI GetVersion() const
    {
        return SALAMATRIX_RUNTIME_VERSION_1_0;
    }

    virtual BOOL WINAPI RegisterAdapter(IRuntimeAdapter* adapter)
    {
        if (adapter == NULL || AdapterCount >= MaxAdapters)
            return FALSE;

        const RuntimeAdapterDescriptor* descriptor = adapter->GetDescriptor();
        if (descriptor == NULL ||
            descriptor->StructSize < sizeof(RuntimeAdapterDescriptor) ||
            descriptor->RuntimeId == NULL ||
            descriptor->RuntimeId[0] == 0 ||
            descriptor->RuntimeVersion == 0)
        {
            return FALSE;
        }

        for (int i = 0; i < AdapterCount; ++i)
        {
            if (Adapters[i] == adapter)
                return TRUE;

            const RuntimeAdapterDescriptor* existing = Adapters[i]->GetDescriptor();
            if (existing != NULL && SameRuntimeId(existing->RuntimeId, descriptor->RuntimeId))
                return FALSE;
        }

        Adapters[AdapterCount++] = adapter;
        return TRUE;
    }

    virtual BOOL WINAPI UnregisterAdapter(IRuntimeAdapter* adapter)
    {
        if (adapter == NULL)
            return FALSE;

        for (int i = 0; i < AdapterCount; ++i)
        {
            if (Adapters[i] == adapter)
            {
                for (int j = i; j + 1 < AdapterCount; ++j)
                    Adapters[j] = Adapters[j + 1];
                Adapters[--AdapterCount] = NULL;
                return TRUE;
            }
        }
        return FALSE;
    }

    virtual int WINAPI GetAdapterCount() const
    {
        return AdapterCount;
    }

    virtual IRuntimeAdapter* WINAPI GetAdapter(int index) const
    {
        if (index < 0 || index >= AdapterCount)
            return NULL;
        return Adapters[index];
    }

    virtual IRuntimeAdapter* WINAPI FindAdapter(const char* runtimeId, DWORD minimumRuntimeVersion) const
    {
        if (runtimeId == NULL)
            return NULL;

        for (int i = 0; i < AdapterCount; ++i)
        {
            const RuntimeAdapterDescriptor* descriptor = Adapters[i]->GetDescriptor();
            if (descriptor != NULL &&
                SameRuntimeId(descriptor->RuntimeId, runtimeId) &&
                descriptor->RuntimeVersion >= minimumRuntimeVersion &&
                Adapters[i]->IsAvailable())
            {
                return Adapters[i];
            }
        }
        return NULL;
    }

    virtual IRuntimeAdapter* WINAPI FindAdapterForEntryPoint(const char* entryPoint) const
    {
        if (entryPoint == NULL)
            return NULL;

        for (int i = 0; i < AdapterCount; ++i)
        {
            if (Adapters[i]->IsAvailable() && Adapters[i]->SupportsEntryPoint(entryPoint))
                return Adapters[i];
        }
        return NULL;
    }
};

class RuntimeServices
{
private:
    LocalUIService UIService;
    Commands::CommandService CommandService;
    FileOperations::FileOperationsService FileOperationsService;
    Automation::ScriptRootAdapter ScriptRoot;
    RuntimeService RuntimeBroker;
    Sides::SidesService SidesService;
    Events::EventService EventService;
    Extensions::ExtensionsService ExtensionsService;
    AI::AssistantService AIService;
    Storage::StorageService StorageService;
    ServiceRegistry Registry;
    CSalamanderGeneralAbstract* General;
    BOOL Registered;
    BOOL HostRegistered;
    BOOL HostUIRegistered;
    BOOL HostCommandsRegistered;
    BOOL HostFileOperationsRegistered;
    BOOL HostAutomationRegistered;
    BOOL HostRuntimeRegistered;
    BOOL HostSidesRegistered;
    BOOL HostEventsRegistered;
    BOOL HostExtensionsRegistered;
    BOOL HostAIRegistered;
    BOOL HostStorageRegistered;

    RuntimeServices(const RuntimeServices&);
    RuntimeServices& operator=(const RuntimeServices&);

    void UnregisterHostServices()
    {
        if (General != NULL)
        {
            if (HostStorageRegistered)
                General->UnregisterService(SALAMATRIX_SERVICE_STORAGE, &StorageService);
            if (HostAIRegistered)
                General->UnregisterService(SALAMATRIX_SERVICE_AI, &AIService);
            if (HostExtensionsRegistered)
                General->UnregisterService(SALAMATRIX_SERVICE_EXTENSIONS, &ExtensionsService);
            if (HostEventsRegistered)
                General->UnregisterService(SALAMATRIX_SERVICE_EVENTS, &EventService);
            if (HostSidesRegistered)
                General->UnregisterService(SALAMATRIX_SERVICE_SIDES, &SidesService);
            if (HostRuntimeRegistered)
                General->UnregisterService(SALAMATRIX_SERVICE_RUNTIME, &RuntimeBroker);
            if (HostAutomationRegistered)
                General->UnregisterService(SALAMATRIX_SERVICE_AUTOMATION_ADAPTER, &ScriptRoot);
            if (HostFileOperationsRegistered)
                General->UnregisterService(SALAMATRIX_SERVICE_FILEOPERATIONS, &FileOperationsService);
            if (HostCommandsRegistered)
                General->UnregisterService(SALAMATRIX_SERVICE_COMMANDS, &CommandService);
            if (HostUIRegistered)
                General->UnregisterService(SALAMATRIX_SERVICE_UI, &UIService);
        }

        HostRuntimeRegistered = FALSE;
        HostSidesRegistered = FALSE;
        HostEventsRegistered = FALSE;
        HostExtensionsRegistered = FALSE;
        HostAIRegistered = FALSE;
        HostStorageRegistered = FALSE;
        HostAutomationRegistered = FALSE;
        HostFileOperationsRegistered = FALSE;
        HostCommandsRegistered = FALSE;
        HostUIRegistered = FALSE;
        HostRegistered = FALSE;
    }

public:
    explicit RuntimeServices(CSalamanderGeneralAbstract* general, BOOL registerHostServices = TRUE)
        : UIService(general),
          CommandService(general),
          FileOperationsService(&CommandService),
          ScriptRoot(&UIService, &CommandService, &FileOperationsService),
          RuntimeBroker(),
          SidesService(general),
          EventService(&SidesService),
          ExtensionsService(),
          AIService(),
          StorageService(),
          Registry(),
          General(general),
          Registered(FALSE),
          HostRegistered(FALSE),
          HostUIRegistered(FALSE),
          HostCommandsRegistered(FALSE),
          HostFileOperationsRegistered(FALSE),
          HostAutomationRegistered(FALSE),
          HostRuntimeRegistered(FALSE),
          HostSidesRegistered(FALSE),
          HostEventsRegistered(FALSE),
          HostExtensionsRegistered(FALSE),
          HostAIRegistered(FALSE),
          HostStorageRegistered(FALSE)
    {
        Registered = TRUE;
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_UI, SALAMATRIX_UI_VERSION_1_0, &UIService, "Salamatrix Framework");
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_COMMANDS, SALAMATRIX_COMMANDS_VERSION_1_0, &CommandService, "Salamatrix Framework");
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_FILEOPERATIONS, SALAMATRIX_FILEOPERATIONS_VERSION_1_0, &FileOperationsService, "Salamatrix Framework");
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_AUTOMATION_ADAPTER, SALAMATRIX_AUTOMATION_VERSION_1_0, &ScriptRoot, "Salamatrix Framework");
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_RUNTIME, SALAMATRIX_RUNTIME_VERSION_1_0, &RuntimeBroker, "Salamatrix Framework");
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_SIDES, SALAMATRIX_SIDES_VERSION_1_0, &SidesService, "Salamatrix Framework");
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_EVENTS, SALAMATRIX_EVENTS_VERSION_1_0, &EventService, "Salamatrix Framework");
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_EXTENSIONS, SALAMATRIX_EXTENSIONS_VERSION_1_0, &ExtensionsService, "Salamatrix Framework");
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_AI, SALAMATRIX_AI_VERSION_1_0, &AIService, "Salamatrix Framework");
        Registered &= Registry.RegisterService(SALAMATRIX_SERVICE_STORAGE, SALAMATRIX_STORAGE_VERSION_1_0, &StorageService, "Salamatrix Framework");

        if (General != NULL && registerHostServices)
        {
            HostUIRegistered = General->RegisterService(SALAMATRIX_SERVICE_UI, SALAMATRIX_UI_VERSION_1_0, &UIService, "Salamatrix Framework");
            if (HostUIRegistered)
                HostCommandsRegistered = General->RegisterService(SALAMATRIX_SERVICE_COMMANDS, SALAMATRIX_COMMANDS_VERSION_1_0, &CommandService, "Salamatrix Framework");
            if (HostCommandsRegistered)
                HostFileOperationsRegistered = General->RegisterService(SALAMATRIX_SERVICE_FILEOPERATIONS, SALAMATRIX_FILEOPERATIONS_VERSION_1_0, &FileOperationsService, "Salamatrix Framework");
            if (HostFileOperationsRegistered)
                HostAutomationRegistered = General->RegisterService(SALAMATRIX_SERVICE_AUTOMATION_ADAPTER, SALAMATRIX_AUTOMATION_VERSION_1_0, &ScriptRoot, "Salamatrix Framework");
            if (HostAutomationRegistered)
                HostRuntimeRegistered = General->RegisterService(SALAMATRIX_SERVICE_RUNTIME, SALAMATRIX_RUNTIME_VERSION_1_0, &RuntimeBroker, "Salamatrix Framework");
            if (HostRuntimeRegistered)
                HostSidesRegistered = General->RegisterService(SALAMATRIX_SERVICE_SIDES, SALAMATRIX_SIDES_VERSION_1_0, &SidesService, "Salamatrix Framework");
            if (HostSidesRegistered)
                HostEventsRegistered = General->RegisterService(SALAMATRIX_SERVICE_EVENTS, SALAMATRIX_EVENTS_VERSION_1_0, &EventService, "Salamatrix Framework");
            if (HostEventsRegistered)
                HostExtensionsRegistered = General->RegisterService(SALAMATRIX_SERVICE_EXTENSIONS, SALAMATRIX_EXTENSIONS_VERSION_1_0, &ExtensionsService, "Salamatrix Framework");
            if (HostExtensionsRegistered)
                HostAIRegistered = General->RegisterService(SALAMATRIX_SERVICE_AI, SALAMATRIX_AI_VERSION_1_0, &AIService, "Salamatrix Framework");
            if (HostAIRegistered)
                HostStorageRegistered = General->RegisterService(SALAMATRIX_SERVICE_STORAGE, SALAMATRIX_STORAGE_VERSION_1_0, &StorageService, "Salamatrix Framework");

            HostRegistered = HostUIRegistered &&
                             HostCommandsRegistered &&
                             HostFileOperationsRegistered &&
                             HostAutomationRegistered &&
                             HostRuntimeRegistered &&
                             HostSidesRegistered &&
                             HostEventsRegistered &&
                             HostExtensionsRegistered &&
                             HostAIRegistered &&
                             HostStorageRegistered;
            if (!HostRegistered)
                UnregisterHostServices();
        }
    }

    ~RuntimeServices()
    {
        UnregisterHostServices();
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

    IRuntimeService* WINAPI Runtimes()
    {
        return &RuntimeBroker;
    }

    Sides::ISidesService* WINAPI Sides()
    {
        return &SidesService;
    }

    Events::EventService* WINAPI Events()
    {
        return &EventService;
    }

    Extensions::ExtensionsService* WINAPI Extensions()
    {
        return &ExtensionsService;
    }

    AI::IAssistantService* WINAPI AI()
    {
        return &AIService;
    }

    Storage::StorageService* WINAPI Storage()
    {
        return &StorageService;
    }
};

} // namespace Runtime
} // namespace Salamatrix
