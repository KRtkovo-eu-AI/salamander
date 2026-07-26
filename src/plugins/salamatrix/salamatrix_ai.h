// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_ai.h
    Provider-neutral contract for the local scripting assistant.
*/

#pragma once

#include <string>
#include <string.h>

#include "salamatrix_runtime_protocol.h"
#include "salamatrix_runtime_api.h"

namespace Salamatrix
{
namespace AI
{

#define SALAMATRIX_SERVICE_AI "Salamatrix.AI"
#define SALAMATRIX_AI_VERSION_1_0 0x00010000

enum AssistantStatus
{
    AssistantStatusNotStarted = 0,
    AssistantStatusSucceeded = 1,
    AssistantStatusUnavailable = 2,
    AssistantStatusInvalidResponse = 3,
    AssistantStatusFailed = 4,
    AssistantStatusCancelled = 5
};

enum AssistantEffectFlags
{
    AssistantEffectNone = 0,
    AssistantEffectReadSelection = 0x00000001,
    AssistantEffectReadMetadata = 0x00000002,
    AssistantEffectRenameFiles = 0x00000004,
    AssistantEffectMoveFiles = 0x00000008,
    AssistantEffectDeleteFiles = 0x00000010,
    AssistantEffectModifyContents = 0x00000020,
    AssistantEffectExecuteExternal = 0x00000040,
    AssistantEffectNetwork = 0x00000080
};

struct AssistantOutputSummary
{
    char Title[256];
    char Description[1024];
    char Script[32768];
    DWORD EffectFlags;
    BOOL ContractValid;

    AssistantOutputSummary()
        : EffectFlags(AssistantEffectNone),
          ContractValid(FALSE)
    {
        Title[0] = '\0';
        Description[0] = '\0';
        Script[0] = '\0';
    }
};

/// Conservative gate used by preview clients before offering a direct Run
/// action. Providers may declare additional effects; only clearly read-only
/// output is considered immediately runnable.
inline BOOL WINAPI IsSafeToRun(const AssistantOutputSummary& summary)
{
    const DWORD unsafe = AssistantEffectDeleteFiles |
                         AssistantEffectExecuteExternal |
                         AssistantEffectNetwork;
    return summary.ContractValid && (summary.EffectFlags & unsafe) == 0;
}

struct AssistantProviderDescriptor
{
    const char* ProviderId;
    const char* DisplayName;
    DWORD ProviderVersion;
    DWORD Flags;
};

struct AssistantRequest
{
    DWORD StructSize;
    const char* Prompt;
    const char* ContextJson;
    const char* ApiVersion;
    DWORD TimeoutMs;
    DWORD MaxOutputBytes;
    /// Optional target runtime hint, e.g. "Python.CPython" or "PowerShell".
    const char* RuntimeId;
    /// Optional existing script to repair or extend during generation.
    const char* ExistingScript;
    /// Optional feedback from the previous preview/run iteration.
    const char* Feedback;

    AssistantRequest()
        : StructSize(sizeof(AssistantRequest)),
          Prompt(NULL),
          ContextJson(NULL),
          ApiVersion("1.0"),
          TimeoutMs(120000),
          MaxOutputBytes(65536),
          RuntimeId(NULL),
          ExistingScript(NULL),
          Feedback(NULL)
    {
    }
};

struct AssistantResponse
{
    DWORD StructSize;
    AssistantStatus Status;
    HRESULT ErrorCode;
    DWORD OutputLength;
    char ResponseJson[65536];
    wchar_t Message[256];
    AssistantOutputSummary Summary;

    AssistantResponse()
        : StructSize(sizeof(AssistantResponse)),
          Status(AssistantStatusNotStarted),
          ErrorCode(S_OK),
          OutputLength(0)
    {
        ResponseJson[0] = '\0';
        Message[0] = L'\0';
    }
};

class IAssistantProvider
{
public:
    virtual const AssistantProviderDescriptor* WINAPI GetDescriptor() const = 0;
    virtual BOOL WINAPI IsAvailable() const = 0;
    virtual BOOL WINAPI Generate(
        const AssistantRequest* request,
        AssistantResponse* response) = 0;

protected:
    virtual ~IAssistantProvider() {}
};

class IAssistantService
{
public:
    virtual DWORD WINAPI GetVersion() const = 0;
    virtual BOOL WINAPI RegisterProvider(IAssistantProvider* provider) = 0;
    virtual BOOL WINAPI UnregisterProvider(IAssistantProvider* provider) = 0;
    virtual int WINAPI GetProviderCount() const = 0;
    virtual IAssistantProvider* WINAPI FindProvider(const char* providerId) const = 0;
    virtual BOOL WINAPI Generate(
        const char* providerId,
        const AssistantRequest* request,
        AssistantResponse* response) = 0;
    virtual const char* WINAPI GetApiDescription() const = 0;
    /// Returns a compact API description for the requested topic.  This is
    /// appended to the ABI so older providers remain source/binary compatible.
    /// A null/empty topic returns the complete description.
    virtual const char* WINAPI GetApiDescriptionSlice(const char* topic) const
    {
        (void)topic;
        return GetApiDescription();
    }

    /// Optional provider enumeration appended to the ABI. Older services
    /// keep returning NULL, while native clients can build a provider picker.
    virtual IAssistantProvider* WINAPI GetProvider(int index) const
    {
        (void)index;
        return NULL;
    }

protected:
    virtual ~IAssistantService() {}
};

class AssistantService : public IAssistantService
{
private:
    enum { MaxProviders = 8 };
    enum { MaxContractVersions = 16 };
    enum { MaxContractSchemas = 16 };
    struct ContractVersion
    {
        char ServiceId[128];
        DWORD Version;

        ContractVersion()
            : Version(0)
        {
            ServiceId[0] = '\0';
        }
    };

    IAssistantProvider* Providers[MaxProviders];
    int ProviderCount;
    ContractVersion ContractVersions[MaxContractVersions];
    int ContractVersionCount;
    struct ContractSchema
    {
        char ServiceId[128];
        char ObjectJson[8192];

        ContractSchema()
        {
            ServiceId[0] = '\0';
            ObjectJson[0] = '\0';
        }
    };
    ContractSchema ContractSchemas[MaxContractSchemas];
    int ContractSchemaCount;
    Runtime::IRuntimeService* RuntimeService;
    mutable std::string ApiDescriptionCache;

    AssistantService(const AssistantService&);
    AssistantService& operator=(const AssistantService&);

    static BOOL CopySummaryString(
        const std::string& value,
        char* destination,
        size_t capacity)
    {
        if (destination == NULL || capacity == 0 || value.size() >= capacity)
            return FALSE;
        memcpy(destination, value.c_str(), value.size() + 1);
        return TRUE;
    }

    static BOOL ValidateResponse(AssistantResponse* response)
    {
        if (response == NULL || response->OutputLength == 0 ||
            response->ResponseJson[0] == '\0')
            return FALSE;
        std::string title;
        std::string description;
        std::string script;
        std::string capabilities;
        std::string effects;
        if (!Runtime::Protocol::Json::FindStringMember(
                response->ResponseJson, "title", &title) ||
            !Runtime::Protocol::Json::FindStringMember(
                response->ResponseJson, "description", &description) ||
            !Runtime::Protocol::Json::FindStringMember(
                response->ResponseJson, "script", &script) ||
            !Runtime::Protocol::Json::FindRawMember(
                response->ResponseJson, "capabilities", &capabilities) ||
            !Runtime::Protocol::Json::FindRawMember(
                response->ResponseJson, "estimatedEffects", &effects) ||
            capabilities.size() < 2 || capabilities[0] != '[' ||
            capabilities[capabilities.size() - 1] != ']' ||
            effects.size() < 2 || effects[0] != '{' ||
            effects[effects.size() - 1] != '}' ||
            !CopySummaryString(title, response->Summary.Title,
                               _countof(response->Summary.Title)) ||
            !CopySummaryString(description, response->Summary.Description,
                               _countof(response->Summary.Description)) ||
            !CopySummaryString(script, response->Summary.Script,
                               _countof(response->Summary.Script)))
            return FALSE;

        response->Summary.EffectFlags = AssistantEffectNone;
        const struct EffectName
        {
            const char* Name;
            DWORD Flag;
        } effectsToFlags[] = {
            {"readSelection", AssistantEffectReadSelection},
            {"readMetadata", AssistantEffectReadMetadata},
            {"renameFiles", AssistantEffectRenameFiles},
            {"moveFiles", AssistantEffectMoveFiles},
            {"deleteFiles", AssistantEffectDeleteFiles},
            {"modifyContents", AssistantEffectModifyContents},
            {"executeExternal", AssistantEffectExecuteExternal},
            {"network", AssistantEffectNetwork}};
        for (int index = 0; index < _countof(effectsToFlags); ++index)
        {
            std::string marker = std::string("\"") +
                                 effectsToFlags[index].Name + "\":";
            size_t position = effects.find(marker);
            if (position != std::string::npos &&
                effects.find("true", position + marker.size()) ==
                    position + marker.size())
                response->Summary.EffectFlags |= effectsToFlags[index].Flag;
        }
        response->Summary.ContractValid = TRUE;
        return TRUE;
    }

public:
    AssistantService()
        : ProviderCount(0),
          ContractVersionCount(0),
          ContractSchemaCount(0),
          RuntimeService(NULL)
    {
        memset(Providers, 0, sizeof(Providers));
    }

    void SetRuntimeService(Runtime::IRuntimeService* service)
    {
        RuntimeService = service;
        ApiDescriptionCache.clear();
    }

    BOOL SetContractVersion(const char* serviceId, DWORD version)
    {
        if (serviceId == NULL || serviceId[0] == '\0')
            return FALSE;
        for (int index = 0; index < ContractVersionCount; ++index)
        {
            if (_stricmp(ContractVersions[index].ServiceId, serviceId) == 0)
            {
                ContractVersions[index].Version = version;
                ApiDescriptionCache.clear();
                return TRUE;
            }
        }
        if (ContractVersionCount >= MaxContractVersions ||
            strlen(serviceId) >= _countof(ContractVersions[0].ServiceId))
            return FALSE;
        memcpy(ContractVersions[ContractVersionCount].ServiceId,
               serviceId, strlen(serviceId) + 1);
        ContractVersions[ContractVersionCount++].Version = version;
        ApiDescriptionCache.clear();
        return TRUE;
    }

    BOOL SetContractSchema(const char* serviceId, const char* objectJson)
    {
        if (serviceId == NULL || serviceId[0] == '\0' ||
            objectJson == NULL || objectJson[0] != '{')
            return FALSE;
        const size_t serviceLength = strlen(serviceId);
        const size_t objectLength = strlen(objectJson);
        if (serviceLength >= _countof(ContractSchemas[0].ServiceId) ||
            objectLength < 2 ||
            objectLength >= _countof(ContractSchemas[0].ObjectJson) ||
            objectJson[objectLength - 1] != '}')
            return FALSE;
        for (int index = 0; index < ContractSchemaCount; ++index)
        {
            if (_stricmp(ContractSchemas[index].ServiceId, serviceId) == 0)
            {
                memcpy(ContractSchemas[index].ObjectJson,
                       objectJson, objectLength + 1);
                ApiDescriptionCache.clear();
                return TRUE;
            }
        }
        if (ContractSchemaCount >= MaxContractSchemas)
            return FALSE;
        memcpy(ContractSchemas[ContractSchemaCount].ServiceId,
               serviceId, serviceLength + 1);
        memcpy(ContractSchemas[ContractSchemaCount].ObjectJson,
               objectJson, objectLength + 1);
        ++ContractSchemaCount;
        ApiDescriptionCache.clear();
        return TRUE;
    }

    virtual DWORD WINAPI GetVersion() const
    {
        return SALAMATRIX_AI_VERSION_1_0;
    }

    virtual BOOL WINAPI RegisterProvider(IAssistantProvider* provider)
    {
        if (provider == NULL || provider->GetDescriptor() == NULL ||
            provider->GetDescriptor()->ProviderId == NULL ||
            FindProvider(provider->GetDescriptor()->ProviderId) != NULL ||
            ProviderCount >= MaxProviders)
            return FALSE;
        Providers[ProviderCount++] = provider;
        return TRUE;
    }

    virtual BOOL WINAPI UnregisterProvider(IAssistantProvider* provider)
    {
        if (provider == NULL)
            return FALSE;
        for (int index = 0; index < ProviderCount; ++index)
        {
            if (Providers[index] == provider)
            {
                for (int move = index; move + 1 < ProviderCount; ++move)
                    Providers[move] = Providers[move + 1];
                Providers[--ProviderCount] = NULL;
                return TRUE;
            }
        }
        return FALSE;
    }

    virtual int WINAPI GetProviderCount() const
    {
        return ProviderCount;
    }

    virtual IAssistantProvider* WINAPI FindProvider(const char* providerId) const
    {
        if (providerId == NULL)
            return NULL;
        for (int index = 0; index < ProviderCount; ++index)
        {
            const AssistantProviderDescriptor* descriptor =
                Providers[index]->GetDescriptor();
            if (descriptor != NULL && descriptor->ProviderId != NULL &&
                strcmp(descriptor->ProviderId, providerId) == 0)
                return Providers[index];
        }
        return NULL;
    }

    virtual IAssistantProvider* WINAPI GetProvider(int index) const
    {
        return index >= 0 && index < ProviderCount ? Providers[index] : NULL;
    }

    virtual BOOL WINAPI Generate(
        const char* providerId,
        const AssistantRequest* request,
        AssistantResponse* response)
    {
        if (response == NULL || response->StructSize < sizeof(*response) ||
            request == NULL || request->StructSize < sizeof(*request))
            return FALSE;
        // An explicit provider id is a strict request.  Automatic selection,
        // however, should be resilient: a configured local endpoint may be
        // temporarily offline while another provider (for example the
        // command wrapper) is available.
        if (providerId != NULL)
        {
            IAssistantProvider* provider = FindProvider(providerId);
            if (provider == NULL || !provider->IsAvailable())
            {
                response->Status = AssistantStatusUnavailable;
                response->ErrorCode = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                return FALSE;
            }
            if (!provider->Generate(request, response))
                return FALSE;
            if (!ValidateResponse(response))
            {
                response->Status = AssistantStatusInvalidResponse;
                response->ErrorCode = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                response->Summary.ContractValid = FALSE;
                return FALSE;
            }
            return TRUE;
        }

        for (int index = 0; index < ProviderCount; ++index)
        {
            IAssistantProvider* provider = Providers[index];
            if (provider == NULL || !provider->IsAvailable())
                continue;
            *response = AssistantResponse();
            if (!provider->Generate(request, response))
                continue;
            if (ValidateResponse(response))
                return TRUE;
            response->Status = AssistantStatusInvalidResponse;
            response->ErrorCode = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            response->Summary.ContractValid = FALSE;
        }
        response->Status = AssistantStatusUnavailable;
        response->ErrorCode = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        return FALSE;
    }

    static const char* GetStaticApiDescription()
    {
        return
            "{\"version\":\"1.0\",\"objects\":{" 
            "\"Salamander.commands\":{\"methods\":[\"execute\",\"register\",\"unregister\"],\"registerFields\":[\"commandId\",\"title\",\"pluginMenu\",\"contextMenu\",\"toolbar\",\"hotKey\"]},"
            "\"Salamander.fileOperations\":{\"methods\":[\"rename\",\"copy\",\"move\",\"delete\",\"createDirectory\",\"refresh\",\"properties\"]},"
            "\"Salamander.sides\":{\"methods\":[\"activeTab\",\"context\",\"tabs\",\"activateTab\",\"changePath\",\"refresh\",\"selectItem\",\"selectAll\",\"focusItem\"],\"contextFields\":[\"path\",\"selectedItems\",\"focusedItem\"],\"tabFields\":[\"id\",\"index\",\"side\",\"pathType\",\"flags\",\"path\"],\"itemFields\":[\"name\",\"path\",\"extension\",\"size\",\"sizeValid\",\"attributes\",\"lastWriteUtc\",\"isDirectory\",\"hidden\",\"link\",\"offline\"]},"
            "\"Salamander.storage\":{\"methods\":[\"get\",\"set\",\"remove\",\"clear\"]},"
            "\"Salamander.events\":{\"methods\":[\"subscribe\",\"unsubscribe\"],\"eventNames\":[\"hostStartup\",\"hostShutdown\",\"settingsChanged\",\"configurationChanged\",\"colorsChanged\",\"panelsSwapped\",\"activePanelChanged\",\"sidePathChanged\",\"sideSelectionChanged\",\"sideTabChanged\",\"sideRefreshed\",\"pathChanged\",\"selectionChanged\",\"tabChanged\"]},"
            "\"Salamander.runtimes\":{\"methods\":[\"list\"],\"fields\":[\"id\",\"name\",\"language\",\"extensions\",\"version\",\"available\"]},"
            "\"Salamander.ui\":{\"methods\":[\"messageBox\",\"inputBox\",\"pickFile\",\"pickFolder\",\"progress\",\"progress.update\",\"progress.step\",\"progress.setTotals\",\"progress.setPositions\",\"progress.cancelled\",\"progress.close\",\"dialog\",\"dialog.add\",\"dialog.get\",\"dialog.set\",\"dialog.validation\",\"dialog.events\",\"dialog.item\",\"dialog.column\",\"dialog.selection\",\"dialog.clearItems\",\"setValidation\",\"onChange\",\"addColumn\",\"setSelectedIndex\"],\"progressStyles\":[1,2],\"layout\":true,\"validation\":true,\"events\":true,\"selection\":true},"
            "\"Salamander.uiOptions\":{\"controlOptions\":[\"readOnly\",\"checked\",\"dialogResult\",\"keepOpen\",\"multiline\"]},"
            "\"Salamander.uiDialogOptions\":{\"fields\":[\"title\",\"width\",\"height\"]},"
            "\"Salamander.clipboard\":{\"methods\":[\"copyText\"]},"
            "\"Salamander.ai\":{\"methods\":[\"generate\",\"preview\",\"api\"],"
            "\"requestFields\":[\"prompt\",\"context\",\"provider\","
            "\"runtime\",\"existingScript\",\"feedback\"]}},"
            "\"assistantOutput\":{\"required\":[\"title\",\"description\",\"capabilities\",\"script\"],\"optional\":[\"runtime\"]}}";
    }

private:
    static void AppendJsonString(std::string& output, const char* value)
    {
        output.push_back('"');
        const char* text = value != NULL ? value : "";
        for (const char* cursor = text; *cursor != '\0'; ++cursor)
        {
            switch (*cursor)
            {
            case '\\': output += "\\\\"; break;
            case '"': output += "\\\""; break;
            case '\r': output += "\\r"; break;
            case '\n': output += "\\n"; break;
            case '\t': output += "\\t"; break;
            default: output.push_back(*cursor); break;
            }
        }
        output.push_back('"');
    }

    void BuildApiDescription() const
    {
        ApiDescriptionCache = GetStaticApiDescription();
        if (ApiDescriptionCache.empty())
            return;

        const size_t rootBrace = ApiDescriptionCache.size() - 1;
        std::string generated;
        generated += ",\"contractVersions\":{";
        for (int index = 0; index < ContractVersionCount; ++index)
        {
            if (index != 0)
                generated.push_back(',');
            AppendJsonString(generated, ContractVersions[index].ServiceId);
            generated += ":";
            char version[32];
            _snprintf_s(version, _countof(version), _TRUNCATE, "%lu.%lu",
                        ContractVersions[index].Version >> 16,
                        ContractVersions[index].Version & 0xffff);
            AppendJsonString(generated, version);
        }
        generated += "},\"runtimeAdapters\":[";
        if (RuntimeService != NULL)
        {
            for (int index = 0; index < RuntimeService->GetAdapterCount(); ++index)
            {
                Runtime::IRuntimeAdapter* adapter = RuntimeService->GetAdapter(index);
                const Runtime::RuntimeAdapterDescriptor* descriptor =
                    adapter != NULL ? adapter->GetDescriptor() : NULL;
                if (descriptor == NULL || descriptor->RuntimeId == NULL)
                    continue;
                if (generated[generated.size() - 1] != '[')
                    generated.push_back(',');
                generated += "{\"id\":";
                AppendJsonString(generated, descriptor->RuntimeId);
                generated += ",\"name\":";
                AppendJsonString(generated, descriptor->DisplayName);
                generated += ",\"language\":";
                AppendJsonString(generated, descriptor->LanguageId);
                generated += ",\"extensions\":";
                AppendJsonString(generated, descriptor->FileExtensions);
                generated += ",\"version\":";
                char version[32];
                _snprintf_s(version, _countof(version), _TRUNCATE, "%lu.%lu",
                            descriptor->RuntimeVersion >> 16,
                            descriptor->RuntimeVersion & 0xffff);
                AppendJsonString(generated, version);
                generated += ",\"available\":";
                generated += adapter->IsAvailable() ? "true" : "false";
                generated += "}";
            }
        }
        generated += "],\"nativeContracts\":{";
        for (int index = 0; index < ContractSchemaCount; ++index)
        {
            if (index != 0)
                generated.push_back(',');
            AppendJsonString(generated, ContractSchemas[index].ServiceId);
            generated += ":";
            generated += ContractSchemas[index].ObjectJson;
        }
        generated += "}";
        ApiDescriptionCache.insert(rootBrace, generated);
    }

public:
    virtual const char* WINAPI GetApiDescription() const
    {
        // Runtime providers can load after the framework plugin, so refresh
        // the generated inventory whenever a live broker is attached.
        if (ApiDescriptionCache.empty() || RuntimeService != NULL)
            BuildApiDescription();
        return ApiDescriptionCache.c_str();
    }

    virtual const char* WINAPI GetApiDescriptionSlice(const char* topic) const
    {
        if (topic == NULL || topic[0] == '\0' || strcmp(topic, "all") == 0)
            return GetApiDescription();
        if (strcmp(topic, "commands") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"commands\",\"objects\":{\"Salamander.commands\":{\"methods\":[\"execute\",\"register\",\"unregister\"],\"registerFields\":[\"commandId\",\"title\",\"pluginMenu\",\"contextMenu\",\"toolbar\",\"hotKey\"]}}}";
        if (strcmp(topic, "fileOperations") == 0 || strcmp(topic, "file_operations") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"fileOperations\",\"objects\":{\"Salamander.fileOperations\":{\"methods\":[\"rename\",\"copy\",\"move\",\"delete\",\"createDirectory\",\"refresh\",\"properties\"]}}}";
        if (strcmp(topic, "sides") == 0 || strcmp(topic, "panels") == 0)
            return "{\"version\":\"1.2\",\"topic\":\"sides\",\"objects\":{\"Salamander.sides\":{\"methods\":[\"activeTab\",\"context\",\"tabs\",\"activateTab\",\"changePath\",\"refresh\",\"selectItem\",\"selectAll\",\"focusItem\"],\"contextFields\":[\"path\",\"selectedItems\",\"focusedItem\"],\"tabFields\":[\"id\",\"index\",\"side\",\"pathType\",\"flags\",\"path\"],\"itemFields\":[\"name\",\"path\",\"extension\",\"size\",\"sizeValid\",\"attributes\",\"lastWriteUtc\",\"isDirectory\",\"hidden\",\"link\",\"offline\"]}}}";
        if (strcmp(topic, "ui") == 0 || strcmp(topic, "dialogs") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"ui\",\"objects\":{\"Salamander.ui\":{\"methods\":[\"messageBox\",\"inputBox\",\"pickFile\",\"pickFolder\",\"progress\",\"progress.update\",\"progress.step\",\"progress.setTotals\",\"progress.setPositions\",\"progress.cancelled\",\"progress.close\",\"dialog\",\"dialog.add\",\"dialog.get\",\"dialog.set\",\"dialog.validation\",\"dialog.events\",\"dialog.item\",\"dialog.column\",\"dialog.selection\",\"dialog.clearItems\"],\"progressStyles\":[1,2],\"layout\":true,\"validation\":true,\"events\":true,\"selection\":true}}}";
        if (strcmp(topic, "uiOptions") == 0 || strcmp(topic, "ui_options") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"uiOptions\",\"objects\":{\"Salamander.ui\":{\"controlOptions\":[\"readOnly\",\"checked\",\"dialogResult\",\"keepOpen\",\"multiline\"]}}}";
        if (strcmp(topic, "uiDialogOptions") == 0 || strcmp(topic, "ui_dialog_options") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"uiDialogOptions\",\"objects\":{\"Salamander.ui\":{\"dialogOptions\":[\"title\",\"width\",\"height\"]}}}";
        if (strcmp(topic, "storage") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"storage\",\"objects\":{\"Salamander.storage\":{\"methods\":[\"get\",\"set\",\"remove\",\"clear\"]}}}";
        if (strcmp(topic, "events") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"events\",\"objects\":{\"Salamander.events\":{\"methods\":[\"subscribe\",\"unsubscribe\"],\"eventNames\":[\"hostStartup\",\"hostShutdown\",\"settingsChanged\",\"configurationChanged\",\"colorsChanged\",\"panelsSwapped\",\"activePanelChanged\",\"sidePathChanged\",\"sideSelectionChanged\",\"sideTabChanged\",\"sideRefreshed\",\"pathChanged\",\"selectionChanged\",\"tabChanged\"]}}}";
        if (strcmp(topic, "runtimes") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"runtimes\",\"objects\":{\"Salamander.runtimes\":{\"methods\":[\"list\"],\"fields\":[\"id\",\"name\",\"language\",\"extensions\",\"version\",\"available\"]}}}";
        if (strcmp(topic, "ai") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"ai\",\"objects\":{\"Salamander.ai\":{\"methods\":[\"generate\",\"preview\",\"api\"],\"requestFields\":[\"prompt\",\"context\",\"provider\",\"runtime\",\"existingScript\",\"feedback\",\"topic\"]}}}";
        return "{\"version\":\"1.0\",\"topic\":\"unknown\",\"objects\":{}}";
    }
};

} // namespace AI
} // namespace Salamatrix
