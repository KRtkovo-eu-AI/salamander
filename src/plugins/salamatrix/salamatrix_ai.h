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

    AssistantRequest()
        : StructSize(sizeof(AssistantRequest)),
          Prompt(NULL),
          ContextJson(NULL),
          ApiVersion("1.0"),
          TimeoutMs(120000),
          MaxOutputBytes(65536)
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

protected:
    virtual ~IAssistantService() {}
};

class AssistantService : public IAssistantService
{
private:
    enum { MaxProviders = 8 };
    IAssistantProvider* Providers[MaxProviders];
    int ProviderCount;

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
        : ProviderCount(0)
    {
        memset(Providers, 0, sizeof(Providers));
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

    virtual BOOL WINAPI Generate(
        const char* providerId,
        const AssistantRequest* request,
        AssistantResponse* response)
    {
        if (response == NULL || response->StructSize < sizeof(*response) ||
            request == NULL || request->StructSize < sizeof(*request))
            return FALSE;
        IAssistantProvider* provider = FindProvider(providerId);
        if (provider == NULL && providerId == NULL)
        {
            for (int index = 0; index < ProviderCount; ++index)
            {
                if (Providers[index]->IsAvailable())
                {
                    provider = Providers[index];
                    break;
                }
            }
        }
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
            response->ErrorCode = E_INVALIDDATA;
            response->Summary.ContractValid = FALSE;
            return FALSE;
        }
        return TRUE;
    }

    virtual const char* WINAPI GetApiDescription() const
    {
        return
            "{\"version\":\"1.0\",\"objects\":{" 
            "\"Salamander.commands\":{\"methods\":[\"execute\",\"register\",\"unregister\"]},"
            "\"Salamander.sides\":{\"methods\":[\"activeTab\"]},"
            "\"Salamander.storage\":{\"methods\":[\"get\",\"set\"]},"
            "\"Salamander.events\":{\"methods\":[\"subscribe\",\"unsubscribe\"]},"
            "\"Salamander.ui\":{\"methods\":[\"messageBox\",\"inputBox\"]},"
            "\"Salamander.ai\":{\"methods\":[\"generate\"]}},"
            "\"assistantOutput\":{\"required\":[\"title\",\"description\",\"capabilities\",\"script\"]}}";
    }
};

} // namespace AI
} // namespace Salamatrix
