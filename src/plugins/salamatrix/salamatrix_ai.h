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

typedef void(WINAPI* AssistantOutputCallback)(
    void* context,
    const char* output,
    DWORD outputLength);

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
    /// Optional live provider console output callback. Appended to preserve
    /// the layout prefix used by older providers.
    AssistantOutputCallback OutputCallback;
    void* OutputContext;

    AssistantRequest()
        : StructSize(sizeof(AssistantRequest)),
          Prompt(NULL),
          ContextJson(NULL),
          ApiVersion("1.0"),
          TimeoutMs(120000),
          MaxOutputBytes(65536),
          RuntimeId(NULL),
          ExistingScript(NULL),
          Feedback(NULL),
          OutputCallback(NULL),
          OutputContext(NULL)
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

struct AssistantValidationResult
{
    BOOL Valid;
    DWORD IssueFlags;
    char Message[512];

    AssistantValidationResult()
        : Valid(FALSE),
          IssueFlags(0)
    {
        Message[0] = '\0';
    }
};

enum AssistantValidationIssueFlags
{
    AssistantValidationIssueNone = 0,
    AssistantValidationIssueShape = 0x00000001,
    AssistantValidationIssueCapability = 0x00000002,
    AssistantValidationIssueEffect = 0x00000004,
    AssistantValidationIssueRuntime = 0x00000008,
    AssistantValidationIssueUnsafeOperation = 0x00000010
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

    /// Validates structured output and its static script contract. Appended
    /// so older AI service providers remain ABI-compatible.
    virtual BOOL WINAPI Validate(
        const AssistantRequest* request,
        AssistantResponse* response,
        AssistantValidationResult* validation)
    {
        (void)request;
        (void)response;
        if (validation != NULL)
            *validation = AssistantValidationResult();
        return FALSE;
    }

    /// Runs generation with a bounded automatic repair loop. Older services
    /// fall back to one Generate call through this default implementation.
    virtual BOOL WINAPI GenerateWithRepair(
        const char* providerId,
        const AssistantRequest* request,
        AssistantResponse* response,
        int maxAttempts)
    {
        (void)maxAttempts;
        return Generate(providerId, request, response);
    }

protected:
    virtual ~IAssistantService() {}
};

// Optional semantic response fields are read from JSON rather than appended
// to AssistantResponse, preserving its ABI layout.
inline BOOL WINAPI AssistantCanImplement(const AssistantResponse& response)
{
    std::string raw;
    if (!Runtime::Protocol::Json::FindRawMember(
            response.ResponseJson, "canImplement", &raw))
        return TRUE;
    return raw != "false" ? TRUE : FALSE;
}

// Keep prompts for small local models bounded by selecting the contract slices
// that match the user's request. This is a client-side helper and does not
// change the provider/service ABI.
inline std::string BuildRelevantApiDescription(
    IAssistantService* service, const char* prompt)
{
    if (service == NULL)
        return "{}";
    std::string text = prompt != NULL ? prompt : "";
    for (size_t index = 0; index < text.size(); ++index)
    {
        if (text[index] >= 'A' && text[index] <= 'Z')
            text[index] = static_cast<char>(text[index] - 'A' + 'a');
    }
    std::string result = "{\"version\":\"1.0\",\"slices\":{";
    int count = 0;
    const auto add = [&](const char* name) {
        const char* slice = service->GetApiDescriptionSlice(name);
        if (slice == NULL || slice[0] == '\0')
            return;
        if (count++ != 0)
            result += ",";
        result += "\"";
        result += name;
        result += "\":";
        result += slice;
    };
    if (text.find("panel") != std::string::npos ||
        text.find("selected") != std::string::npos ||
        text.find("file") != std::string::npos ||
        text.find("directory") != std::string::npos)
        add("sides");
    if (text.find("rename") != std::string::npos ||
        text.find("copy") != std::string::npos ||
        text.find("move") != std::string::npos ||
        text.find("delete") != std::string::npos ||
        text.find("command") != std::string::npos)
    {
        add("commands");
        add("fileOperations");
    }
    if (text.find("dialog") != std::string::npos ||
        text.find("ui") != std::string::npos ||
        text.find("progress") != std::string::npos ||
        text.find("show") != std::string::npos)
        add("ui");
    if (text.find("storage") != std::string::npos ||
        text.find("setting") != std::string::npos)
        add("storage");
    if (text.find("event") != std::string::npos ||
        text.find("change") != std::string::npos)
        add("events");
    if (text.find("runtime") != std::string::npos ||
        text.find("python") != std::string::npos ||
        text.find("powershell") != std::string::npos ||
        text.find("javascript") != std::string::npos ||
        text.find("node") != std::string::npos)
        add("runtimes");
    if (count == 0)
        add("all");
    result += "}}";
    return result;
}

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
            std::string rawEffect;
            if (Runtime::Protocol::Json::FindRawMember(
                    effects.c_str(), effectsToFlags[index].Name, &rawEffect) &&
                rawEffect == "true")
                response->Summary.EffectFlags |= effectsToFlags[index].Flag;
        }
        response->Summary.ContractValid = TRUE;
        return TRUE;
    }

    static BOOL SetValidationFailure(
        AssistantValidationResult* validation,
        DWORD issue,
        const char* message)
    {
        if (validation != NULL)
        {
            validation->Valid = FALSE;
            validation->IssueFlags |= issue;
            if (message != NULL)
                strncpy_s(validation->Message,
                          _countof(validation->Message),
                          message,
                          _TRUNCATE);
        }
        return FALSE;
    }

    static void CopyValidationMessage(
        AssistantResponse* response,
        const AssistantValidationResult& validation)
    {
        if (response == NULL || validation.Message[0] == '\0')
            return;
        if (MultiByteToWideChar(
                CP_UTF8, 0, validation.Message, -1,
                response->Message, _countof(response->Message)) == 0)
        {
            const wchar_t fallback[] = L"Static Salamatrix validation failed.";
            memcpy(response->Message, fallback, sizeof(fallback));
        }
    }

    static BOOL IsKnownCapability(const std::string& capability)
    {
        static const char* const known[] = {
            "panels.read", "panels.write", "ui.dialogs", "commands",
            "file-operations", "storage", "events", "ai", "clipboard",
            "runtimes"};
        for (int index = 0; index < _countof(known); ++index)
        {
            if (capability == known[index])
                return TRUE;
        }
        return FALSE;
    }

    static BOOL ValidateCapabilityArray(
        const std::string& raw,
        AssistantValidationResult* validation)
    {
        size_t position = 0;
        Runtime::Protocol::Json::SkipWhitespace(raw, &position);
        if (position >= raw.size() || raw[position++] != '[')
            return SetValidationFailure(validation,
                                        AssistantValidationIssueCapability,
                                        "capabilities must be a JSON array");
        Runtime::Protocol::Json::SkipWhitespace(raw, &position);
        if (position < raw.size() && raw[position] == ']')
            return TRUE;
        for (;;)
        {
            Runtime::Protocol::Json::SkipWhitespace(raw, &position);
            std::string capability;
            if (!Runtime::Protocol::Json::ReadString(raw, &position, &capability) ||
                !IsKnownCapability(capability))
                return SetValidationFailure(validation,
                                            AssistantValidationIssueCapability,
                                            "capabilities contains an unsupported value");
            Runtime::Protocol::Json::SkipWhitespace(raw, &position);
            if (position >= raw.size())
                return SetValidationFailure(validation,
                                            AssistantValidationIssueCapability,
                                            "capabilities array is unterminated");
            if (raw[position] == ']')
                return TRUE;
            if (raw[position++] != ',')
                return SetValidationFailure(validation,
                                            AssistantValidationIssueCapability,
                                            "capabilities must contain strings");
        }
    }

    static BOOL ValidateStringArray(
        const std::string& raw,
        AssistantValidationResult* validation)
    {
        size_t position = 0;
        Runtime::Protocol::Json::SkipWhitespace(raw, &position);
        if (position >= raw.size() || raw[position++] != '[')
            return SetValidationFailure(validation,
                                        AssistantValidationIssueCapability,
                                        "missingCapabilities must be a JSON array");
        Runtime::Protocol::Json::SkipWhitespace(raw, &position);
        if (position < raw.size() && raw[position] == ']')
            return TRUE;
        for (;;)
        {
            Runtime::Protocol::Json::SkipWhitespace(raw, &position);
            std::string value;
            if (!Runtime::Protocol::Json::ReadString(raw, &position, &value))
                return SetValidationFailure(validation,
                                            AssistantValidationIssueCapability,
                                            "missingCapabilities must contain strings");
            Runtime::Protocol::Json::SkipWhitespace(raw, &position);
            if (position >= raw.size())
                return SetValidationFailure(validation,
                                            AssistantValidationIssueCapability,
                                            "missingCapabilities array is unterminated");
            if (raw[position] == ']')
                return TRUE;
            if (raw[position++] != ',')
                return SetValidationFailure(validation,
                                            AssistantValidationIssueCapability,
                                            "missingCapabilities must contain strings");
        }
    }

    static BOOL ValidateOutput(
        const AssistantRequest* request,
        AssistantResponse* response,
        AssistantValidationResult* validation)
    {
        if (validation != NULL)
            *validation = AssistantValidationResult();
        if (response == NULL || !ValidateResponse(response))
            return SetValidationFailure(validation,
                                        AssistantValidationIssueShape,
                                        "assistant output does not match the Salamatrix response contract");

        std::string canImplement;
        if (!Runtime::Protocol::Json::FindRawMember(
                response->ResponseJson, "canImplement", &canImplement) ||
            (canImplement != "true" && canImplement != "false"))
            return SetValidationFailure(validation,
                                        AssistantValidationIssueShape,
                                        "canImplement is required and must be a boolean");
        std::string missingCapabilities;
        if (Runtime::Protocol::Json::FindRawMember(
                response->ResponseJson, "missingCapabilities", &missingCapabilities) &&
            !ValidateStringArray(missingCapabilities, validation))
            return FALSE;
        if (canImplement == "false" && missingCapabilities.empty())
            return SetValidationFailure(validation,
                                        AssistantValidationIssueCapability,
                                        "missingCapabilities is required when canImplement is false");
        if (canImplement == "true")
        {
            std::string script(response->Summary.Script);
            size_t first = script.find_first_not_of(" \t\r\n");
            size_t last = script.find_last_not_of(" \t\r\n");
            if (first == std::string::npos ||
                (last == first + 2 && script.compare(first, 3, "...") == 0))
                return SetValidationFailure(
                    validation,
                    AssistantValidationIssueShape,
                    "script must contain executable source code, not a placeholder");
        }

        std::string capabilities;
        std::string effects;
        if (!Runtime::Protocol::Json::FindRawMember(
                response->ResponseJson, "capabilities", &capabilities) ||
            !ValidateCapabilityArray(capabilities, validation) ||
            !Runtime::Protocol::Json::FindRawMember(
                response->ResponseJson, "estimatedEffects", &effects) ||
            effects.size() < 2 || effects[0] != '{' ||
            effects[effects.size() - 1] != '}')
        {
            if (validation != NULL && validation->Message[0] != '\0')
                return FALSE;
            return SetValidationFailure(validation,
                                        AssistantValidationIssueEffect,
                                        "estimatedEffects must be a JSON object");
        }

        static const char* const effectNames[] = {
            "readSelection", "readMetadata", "renameFiles", "moveFiles",
            "deleteFiles", "modifyContents", "executeExternal", "network"};
        for (int index = 0; index < _countof(effectNames); ++index)
        {
            std::string raw;
            if (!Runtime::Protocol::Json::FindRawMember(
                    effects.c_str(), effectNames[index], &raw))
                return SetValidationFailure(validation,
                                            AssistantValidationIssueEffect,
                                            "estimatedEffects must contain every declared effect as a boolean");
            if (raw != "true" && raw != "false")
                return SetValidationFailure(validation,
                                            AssistantValidationIssueEffect,
                                            "estimatedEffects values must be booleans");
        }

        std::string runtime;
        BOOL hasRuntime = Runtime::Protocol::Json::FindStringMember(
            response->ResponseJson, "runtime", &runtime);
        if (hasRuntime && runtime.empty())
            return SetValidationFailure(validation,
                                        AssistantValidationIssueRuntime,
                                        "runtime must not be empty");
        if (request != NULL && request->RuntimeId != NULL &&
            request->RuntimeId[0] != '\0' &&
            (!hasRuntime || runtime != request->RuntimeId))
            return SetValidationFailure(validation,
                                        AssistantValidationIssueRuntime,
                                        "assistant output runtime differs from the requested runtime");

        struct UnsafeMarker
        {
            const char* Text;
            DWORD Effect;
        };
        static const UnsafeMarker markers[] = {
            {"child_process", AssistantEffectExecuteExternal},
            {"subprocess", AssistantEffectExecuteExternal},
            {"Start-Process", AssistantEffectExecuteExternal},
            {"shell_exec", AssistantEffectExecuteExternal},
            {"Invoke-WebRequest", AssistantEffectNetwork},
            {"Invoke-RestMethod", AssistantEffectNetwork},
            {"urllib.", AssistantEffectNetwork},
            {"requests.", AssistantEffectNetwork}};
        const std::string script(response->Summary.Script);
        if (canImplement == "true" &&
            script.find("this.selectedItems") != std::string::npos)
            return SetValidationFailure(
                validation,
                AssistantValidationIssueShape,
                "selected files must come from Salamander.sides.context; this.selectedItems does not exist");
        if (canImplement == "true" &&
            script.find("selectedItems") != std::string::npos &&
            script.find("sides.context") == std::string::npos)
            return SetValidationFailure(
                validation,
                AssistantValidationIssueShape,
                "script uses selectedItems without obtaining it from Salamander.sides.context");
        if (canImplement == "true" &&
            (script.find("sides.context") != std::string::npos ||
             script.find("selectedItems") != std::string::npos) &&
            (response->Summary.EffectFlags & AssistantEffectReadSelection) == 0)
            return SetValidationFailure(
                validation,
                AssistantValidationIssueEffect,
                "script reads the panel selection but readSelection is false");
        static const char* const contentWriters[] = {
            "writeFile(", "writeFileSync(", "file_put_contents(",
            "Set-Content", "Out-File"};
        for (int index = 0; index < _countof(contentWriters); ++index)
        {
            if (canImplement == "true" &&
                script.find(contentWriters[index]) != std::string::npos &&
                (response->Summary.EffectFlags &
                 AssistantEffectModifyContents) == 0)
                return SetValidationFailure(
                    validation,
                    AssistantValidationIssueEffect,
                    "script writes file contents but modifyContents is false");
        }

        if (canImplement == "true" &&
            runtime == "JavaScript.Node" &&
            script.find("require(") != std::string::npos)
            return SetValidationFailure(
                validation,
                AssistantValidationIssueRuntime,
                "JavaScript.Node scripts run as ECMAScript modules; use import instead of require");

        std::string task = request != NULL && request->Prompt != NULL
                               ? request->Prompt
                               : "";
        for (size_t index = 0; index < task.size(); ++index)
        {
            if (task[index] >= 'A' && task[index] <= 'Z')
                task[index] =
                    static_cast<char>(task[index] - 'A' + 'a');
        }
        const BOOL md5Task =
            runtime == "JavaScript.Node" &&
            task.find("md5") != std::string::npos;
        const BOOL md5SidecarTask =
            md5Task &&
            (task.find(".md5") != std::string::npos ||
             task.find("sidecar") != std::string::npos ||
             task.find("write") != std::string::npos ||
             task.find("save") != std::string::npos ||
             task.find("create") != std::string::npos ||
             task.find("vytvo") != std::string::npos ||
             task.find("soubor") != std::string::npos);
        if (md5Task && canImplement == "false")
        {
            return SetValidationFailure(
                validation,
                AssistantValidationIssueCapability,
                "MD5 processing of selected file paths is implementable with Salamander.sides.context and Node built-ins");
        }
        if (md5SidecarTask)
        {
            if (script.find("sides.context") == std::string::npos ||
                script.find("createHash") == std::string::npos ||
                (script.find("\"md5\"") == std::string::npos &&
                 script.find("'md5'") == std::string::npos) ||
                script.find("writeFile") == std::string::npos ||
                script.find(".md5") == std::string::npos)
                return SetValidationFailure(
                    validation,
                    AssistantValidationIssueShape,
                    "MD5 sidecar script must read selected paths, hash each file, and write the requested .md5 file");
        }

        for (int index = 0; index < _countof(markers); ++index)
        {
            if (canImplement == "true" &&
                script.find(markers[index].Text) != std::string::npos &&
                (response->Summary.EffectFlags & markers[index].Effect) == 0)
                return SetValidationFailure(
                    validation,
                    AssistantValidationIssueUnsafeOperation,
                    "script contains an external or network operation not declared in estimatedEffects");
        }

        if (validation != NULL)
        {
            validation->Valid = TRUE;
            validation->IssueFlags = AssistantValidationIssueNone;
            validation->Message[0] = '\0';
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

    virtual BOOL WINAPI Validate(
        const AssistantRequest* request,
        AssistantResponse* response,
        AssistantValidationResult* validation)
    {
        return ValidateOutput(request, response, validation);
    }

    virtual BOOL WINAPI GenerateWithRepair(
        const char* providerId,
        const AssistantRequest* request,
        AssistantResponse* response,
        int maxAttempts)
    {
        if (request == NULL || response == NULL)
            return FALSE;
        if (maxAttempts < 1)
            maxAttempts = 1;
        if (maxAttempts > 4)
            maxAttempts = 4;

        AssistantRequest retry = *request;
        std::string previousScript;
        std::string repairFeedback;
        for (int attempt = 0; attempt < maxAttempts; ++attempt)
        {
            retry.ExistingScript = previousScript.empty()
                                       ? request->ExistingScript
                                       : previousScript.c_str();
            retry.Feedback = repairFeedback.empty()
                                 ? request->Feedback
                                 : repairFeedback.c_str();
            AssistantResponse candidate;
            if (Generate(providerId, &retry, &candidate))
            {
                *response = candidate;
                return TRUE;
            }
            *response = candidate;
            if (candidate.Status != AssistantStatusInvalidResponse ||
                attempt + 1 >= maxAttempts)
                return FALSE;
            if (candidate.Summary.Script[0] != '\0')
                previousScript.assign(candidate.Summary.Script);
            AssistantValidationResult validation;
            ValidateOutput(&retry, &candidate, &validation);
            repairFeedback =
                "The previous response failed static Salamatrix validation. "
                "Specific validation error: " +
                (validation.Message[0] != '\0'
                     ? std::string(validation.Message)
                     : std::string("the response does not match the contract")) +
                ". "
                "Return corrected JSON, declare every external or network "
                "operation in estimatedEffects, or set canImplement to false "
                "and list missingCapabilities when the installed API cannot "
                "perform the requested task. capabilities must be a JSON "
                "array and estimatedEffects must be a JSON object with "
                "boolean values. Allowed capabilities are exactly: "
                "panels.read, panels.write, ui.dialogs, commands, "
                "file-operations, storage, events, ai, clipboard, runtimes. "
                "canImplement and missingCapabilities belong at the top level, "
                "not inside estimatedEffects.";
        }
        return FALSE;
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
            AssistantValidationResult validation;
            if (!ValidateOutput(request, response, &validation))
            {
                response->Status = AssistantStatusInvalidResponse;
                response->ErrorCode = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                response->Summary.ContractValid = FALSE;
                CopyValidationMessage(response, validation);
                return FALSE;
            }
            return TRUE;
        }

        BOOL hadInvalidResponse = FALSE;
        AssistantResponse lastInvalidResponse;
        for (int index = 0; index < ProviderCount; ++index)
        {
            IAssistantProvider* provider = Providers[index];
            if (provider == NULL || !provider->IsAvailable())
                continue;
            *response = AssistantResponse();
            if (!provider->Generate(request, response))
                continue;
            AssistantValidationResult validation;
            if (ValidateOutput(request, response, &validation))
                return TRUE;
            response->Status = AssistantStatusInvalidResponse;
            response->ErrorCode = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            response->Summary.ContractValid = FALSE;
            CopyValidationMessage(response, validation);
            lastInvalidResponse = *response;
            hadInvalidResponse = TRUE;
        }
        if (hadInvalidResponse)
        {
            *response = lastInvalidResponse;
            return FALSE;
        }
        response->Status = AssistantStatusUnavailable;
        response->ErrorCode = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        return FALSE;
    }

    static const char* GetStaticApiDescription()
    {
        return
            "{\"version\":\"1.0\",\"objects\":{" 
            "\"Salamander\":{\"fields\":[\"command_id\",\"command_handler\"]},"
            "\"Salamander.commands\":{\"methods\":[\"execute\",\"register\",\"unregister\",\"setState\"],\"registerFields\":[\"commandId\",\"title\",\"handler\",\"pluginMenu\",\"contextMenu\",\"toolbar\",\"hotKey\",\"enabled\",\"visible\"],\"stateFields\":[\"commandId\",\"enabled\",\"visible\"]},"
            "\"Salamander.fileOperations\":{\"methods\":[\"rename\",\"copy\",\"move\",\"delete\",\"createDirectory\",\"refresh\",\"properties\"]},"
            "\"Salamander.sides\":{\"methods\":[\"activeTab\",\"context\",\"tabs\",\"activateTab\",\"changePath\",\"refresh\",\"selectItem\",\"selectAll\",\"focusItem\",\"createTab\",\"closeTab\",\"reorderTab\",\"moveTab\",\"setDetached\"],\"contextFields\":[\"path\",\"selectedItems\",\"focusedItem\"],\"tabFields\":[\"id\",\"index\",\"side\",\"pathType\",\"flags\",\"path\"],\"itemFields\":[\"name\",\"path\",\"extension\",\"size\",\"sizeValid\",\"attributes\",\"lastWriteUtc\",\"isDirectory\",\"hidden\",\"link\",\"offline\"]},"
            "\"Salamander.storage\":{\"methods\":[\"get\",\"set\",\"remove\",\"clear\",\"keys\",\"schema\"],\"valueTypes\":[\"string\",\"integer\",\"boolean\"],\"getResultFields\":[\"type\",\"value\"],\"keyFields\":[\"key\",\"type\"],\"schemaFields\":[\"key\",\"type\",\"hasDefault\",\"default\"]},"
            "\"Salamander.events\":{\"methods\":[\"subscribe\",\"unsubscribe\"],\"eventNames\":[\"hostStartup\",\"hostShutdown\",\"settingsChanged\",\"configurationChanged\",\"colorsChanged\",\"panelsSwapped\",\"activePanelChanged\",\"sidePathChanged\",\"sideSelectionChanged\",\"sideTabChanged\",\"sideRefreshed\",\"pathChanged\",\"selectionChanged\",\"tabChanged\",\"tabCreated\",\"tabClosed\",\"tabReordered\",\"windowDetached\",\"windowAttached\",\"fileChanged\"]},"
            "\"Salamander.runtimes\":{\"methods\":[\"list\"],\"fields\":[\"id\",\"name\",\"language\",\"extensions\",\"version\",\"available\"]},"
            "\"Salamander.ui\":{\"methods\":[\"messageBox\",\"inputBox\",\"notify\",\"pickFile\",\"pickFolder\",\"progress\",\"progress.update\",\"progress.step\",\"progress.setTotals\",\"progress.setPositions\",\"progress.cancelled\",\"progress.close\",\"dialog\",\"dialog.add\",\"dialog.get\",\"dialog.set\",\"dialog.validation\",\"dialog.events\",\"dialog.item\",\"dialog.column\",\"dialog.selection\",\"dialog.clearItems\",\"setValidation\",\"onChange\",\"addColumn\",\"setSelectedIndex\"],\"controlKinds\":[\"label\",\"textbox\",\"checkbox\",\"radio\",\"combobox\",\"button\",\"listview\",\"treeview\",\"tabcontrol\",\"folderpicker\",\"filepicker\"],\"progressStyles\":[1,2],\"layout\":true,\"validation\":true,\"events\":true,\"selection\":true,\"notifications\":true},"
            "\"Salamander.uiOptions\":{\"controlOptions\":[\"readOnly\",\"checked\",\"dialogResult\",\"keepOpen\",\"multiline\"],\"filePickerOptions\":[\"filter\",\"save\"]},"
            "\"Salamander.uiDialogOptions\":{\"fields\":[\"title\",\"width\",\"height\"]},"
            "\"Salamander.clipboard\":{\"methods\":[\"copyText\"]},"
            "\"Salamander.ai\":{\"methods\":[\"generate\",\"preview\",\"api\"],"
            "\"requestFields\":[\"prompt\",\"context\",\"provider\","
            "\"runtime\",\"existingScript\",\"feedback\"]}},"
            "\"assistantOutput\":{\"required\":[\"title\",\"description\",\"capabilities\",\"estimatedEffects\",\"canImplement\",\"script\"],\"optional\":[\"runtime\",\"missingCapabilities\"]}}";
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
            return "{\"version\":\"1.0\",\"topic\":\"commands\",\"objects\":{\"Salamander.commands\":{\"methods\":{\"execute\":{\"arguments\":[\"commandId\"],\"result\":\"string\"},\"register\":{\"arguments\":[\"commandId\",\"title\",\"pluginMenu=true\",\"contextMenu=false\",\"hotKey=0\",\"toolbar=false\",\"handler=''\",\"enabled=true\",\"visible=true\"],\"result\":\"boolean\"},\"unregister\":{\"arguments\":[\"commandId\"],\"result\":\"boolean\"},\"setState\":{\"arguments\":[\"commandId\",\"enabled?\",\"visible?\"],\"result\":\"boolean\"}},\"registerFields\":[\"commandId\",\"title\",\"handler\",\"pluginMenu\",\"contextMenu\",\"toolbar\",\"hotKey\",\"enabled\",\"visible\"],\"stateFields\":[\"commandId\",\"enabled\",\"visible\"]}}}";
        if (strcmp(topic, "execution") == 0 || strcmp(topic, "commandContext") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"execution\",\"objects\":{\"Salamander\":{\"fields\":[\"command_id\",\"command_handler\"]}}}";
        if (strcmp(topic, "fileOperations") == 0 || strcmp(topic, "file_operations") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"fileOperations\",\"objects\":{\"Salamander.fileOperations\":{\"methods\":[\"rename\",\"copy\",\"move\",\"delete\",\"createDirectory\",\"refresh\",\"properties\"],\"arguments\":\"none\",\"result\":\"string\",\"semantics\":\"Invokes Salamander interactive commands for the current selection; it does not accept arbitrary paths. Use runtime filesystem libraries for direct path-based content processing.\"}}}";
        if (strcmp(topic, "sides") == 0 || strcmp(topic, "panels") == 0)
            return "{\"version\":\"1.3\",\"topic\":\"sides\",\"objects\":{\"Salamander.sides\":{\"methods\":[\"activeTab\",\"context\",\"tabs\",\"activateTab\",\"changePath\",\"refresh\",\"selectItem\",\"selectAll\",\"focusItem\",\"createTab\",\"closeTab\",\"reorderTab\",\"moveTab\",\"setDetached\"],\"contextCall\":{\"arguments\":[\"source|target|left|right\"],\"async\":true,\"resultFields\":[\"path\",\"pathType\",\"selectedCount\",\"selectedItems\",\"focusedItem\"]},\"contextFields\":[\"path\",\"selectedItems\",\"focusedItem\"],\"tabFields\":[\"id\",\"index\",\"side\",\"pathType\",\"flags\",\"path\"],\"itemFields\":[\"name\",\"path\",\"extension\",\"size\",\"sizeValid\",\"attributes\",\"lastWriteUtc\",\"isDirectory\",\"hidden\",\"link\",\"offline\"]}},\"javascriptNodeExample\":\"const context = await Salamander.sides.context(\\\"source\\\"); for (const item of context.selectedItems) console.log(item.path);\"}";
        if (strcmp(topic, "ui") == 0 || strcmp(topic, "dialogs") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"ui\",\"objects\":{\"Salamander.ui\":{\"methods\":{\"messageBox\":{\"arguments\":[\"message\",\"title='Salamander'\"],\"result\":\"integer\"},\"notify\":{\"arguments\":[\"message\",\"title='Salamander'\",\"timeoutMs=5000\"],\"result\":\"boolean\"},\"inputBox\":{\"arguments\":[\"prompt\",\"title='Salamander'\",\"initial=''\"],\"resultFields\":[\"accepted\",\"value\"]},\"pickFile\":{\"arguments\":[\"save=false\",\"title=''\",\"filter=''\",\"initial=''\"],\"resultFields\":[\"selected\",\"path\"]},\"pickFolder\":{\"arguments\":[\"title=''\",\"initial=''\"],\"resultFields\":[\"selected\",\"path\"]},\"progress\":{\"arguments\":[\"title\",\"total\",\"twoProgressBars=false\",\"fileProgress=false\",\"cancelEnabled=true\",\"total2?\"],\"objectMethods\":[\"update\",\"step\",\"setTotals\",\"setPositions\",\"setTitle\",\"setCancelEnabled\",\"isCancelled\",\"close\"]},\"dialog\":{\"arguments\":[\"title\",\"width=320\",\"height=180\"],\"objectMethods\":[\"addControl\",\"addLabel\",\"addTextBox\",\"addFolderPicker\",\"addFilePicker\",\"addCheckBox\",\"addRadioButton\",\"addComboBox\",\"addListView\",\"addTreeView\",\"addTabControl\",\"addButton\",\"addItem\",\"addColumn\",\"setSelectedIndex\",\"clearItems\",\"setValidation\",\"onChange\",\"offChange\",\"show\",\"get\",\"set\",\"close\"]}},\"wireMethods\":[\"dialog.validation\",\"dialog.events\",\"dialog.item\",\"dialog.column\",\"dialog.selection\",\"dialog.clearItems\"],\"controlKinds\":[\"label\",\"textbox\",\"checkbox\",\"radio\",\"combobox\",\"button\",\"listview\",\"treeview\",\"tabcontrol\",\"folderpicker\",\"filepicker\"],\"controlOptions\":[\"readOnly\",\"checked\",\"dialogResult\",\"keepOpen\",\"multiline\",\"filter\",\"save\"],\"layoutFields\":[\"x\",\"y\",\"width\",\"height\"]}}}";
        if (strcmp(topic, "uiOptions") == 0 || strcmp(topic, "ui_options") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"uiOptions\",\"objects\":{\"Salamander.ui\":{\"controlOptions\":[\"readOnly\",\"checked\",\"dialogResult\",\"keepOpen\",\"multiline\"],\"filePickerOptions\":[\"filter\",\"save\"]}}}";
        if (strcmp(topic, "uiDialogOptions") == 0 || strcmp(topic, "ui_dialog_options") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"uiDialogOptions\",\"objects\":{\"Salamander.ui\":{\"dialogOptions\":[\"title\",\"width\",\"height\"]}}}";
        if (strcmp(topic, "storage") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"storage\",\"objects\":{\"Salamander.storage\":{\"methods\":{\"get\":{\"arguments\":[\"key\",\"default?\"],\"result\":\"typed value or default\"},\"set\":{\"arguments\":[\"key\",\"value\"]},\"remove\":{\"arguments\":[\"key\"],\"result\":\"boolean\"},\"clear\":{\"arguments\":[],\"result\":\"boolean\"},\"keys\":{\"arguments\":[],\"resultFields\":[\"key\",\"type\"]},\"schema\":{\"arguments\":[],\"resultFields\":[\"key\",\"type\",\"hasDefault\",\"default\"]}},\"valueTypes\":[\"string\",\"integer\",\"boolean\"],\"scope\":\"current extension package\"}}}";
        if (strcmp(topic, "events") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"events\",\"objects\":{\"Salamander.events\":{\"methods\":{\"subscribe\":{\"arguments\":[\"eventName\",\"handler\"],\"result\":\"subscriptionId string\"},\"unsubscribe\":{\"arguments\":[\"subscriptionId\"]}},\"eventNames\":[\"hostStartup\",\"hostShutdown\",\"settingsChanged\",\"configurationChanged\",\"colorsChanged\",\"panelsSwapped\",\"activePanelChanged\",\"sidePathChanged\",\"sideSelectionChanged\",\"sideTabChanged\",\"sideRefreshed\",\"pathChanged\",\"selectionChanged\",\"tabChanged\",\"tabCreated\",\"tabClosed\",\"tabReordered\",\"windowDetached\",\"windowAttached\",\"fileChanged\"],\"semantics\":\"Future events are useful for persistent extensions; a one-shot script normally exits before future events arrive.\"}}}";
        if (strcmp(topic, "runtimes") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"runtimes\",\"objects\":{\"Salamander.runtimes\":{\"methods\":{\"list\":{\"arguments\":[],\"result\":\"runtime records\"}},\"fields\":[\"id\",\"name\",\"language\",\"extensions\",\"version\",\"available\"],\"runtimeIds\":[\"JavaScript.Node\",\"Python.CPython\",\"PowerShell\",\"PHP.CLI\"]}}}";
        if (strcmp(topic, "ai") == 0)
            return "{\"version\":\"1.0\",\"topic\":\"ai\",\"objects\":{\"Salamander.ai\":{\"methods\":[\"generate\",\"preview\",\"api\"],\"requestFields\":[\"prompt\",\"context\",\"provider\",\"runtime\",\"existingScript\",\"feedback\",\"topic\"],\"responseRequiredFields\":[\"title\",\"description\",\"capabilities\",\"estimatedEffects\",\"canImplement\",\"script\"],\"responseOptionalFields\":[\"runtime\",\"missingCapabilities\"]}}}";
        return "{\"version\":\"1.0\",\"topic\":\"unknown\",\"objects\":{}}";
    }
};

} // namespace AI
} // namespace Salamatrix
