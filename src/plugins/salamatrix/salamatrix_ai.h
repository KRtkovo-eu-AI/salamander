// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_ai.h
    Provider-neutral contract for the local scripting assistant.
*/

#pragma once

#include <string.h>

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
        return provider->Generate(request, response);
    }

    virtual const char* WINAPI GetApiDescription() const
    {
        return
            "{\"version\":\"1.0\",\"objects\":{" 
            "\"Salamander.commands\":{\"methods\":[\"execute\"]},"
            "\"Salamander.sides\":{\"methods\":[\"activeTab\"]},"
            "\"Salamander.storage\":{\"methods\":[\"get\",\"set\"]},"
            "\"Salamander.events\":{\"methods\":[\"subscribe\",\"unsubscribe\"]},"
            "\"Salamander.ui\":{\"methods\":[\"messageBox\"]}},"
            "\"assistantOutput\":{\"required\":[\"title\",\"description\",\"capabilities\",\"script\"]}}";
    }
};

} // namespace AI
} // namespace Salamatrix
